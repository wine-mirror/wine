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

struct msghdr;
struct _DISPATCHER_CONTEXT;

/* increment this when you change the function table */
#define NTDLL_UNIXLIB_VERSION 84

struct unix_funcs
{
    /* Nt* functions */
    NTSTATUS      (WINAPI *NtAllocateVirtualMemory)( HANDLE process, PVOID *ret, ULONG_PTR zero_bits,
                                                     SIZE_T *size_ptr, ULONG type, ULONG protect );
    NTSTATUS      (WINAPI *NtAreMappedFilesTheSame)(PVOID addr1, PVOID addr2);
    NTSTATUS      (WINAPI *NtClose)( HANDLE handle );
    NTSTATUS      (WINAPI *NtCreateMailslotFile)( HANDLE *handle, ULONG access, OBJECT_ATTRIBUTES *attr,
                                                  IO_STATUS_BLOCK *io, ULONG options, ULONG quota,
                                                  ULONG msg_size, LARGE_INTEGER *timeout );
    NTSTATUS      (WINAPI *NtCreateNamedPipeFile)( HANDLE *handle, ULONG access, OBJECT_ATTRIBUTES *attr,
                                                   IO_STATUS_BLOCK *io, ULONG sharing, ULONG dispo,
                                                   ULONG options, ULONG pipe_type, ULONG read_mode,
                                                   ULONG completion_mode, ULONG max_inst,
                                                   ULONG inbound_quota, ULONG outbound_quota,
                                                   LARGE_INTEGER *timeout );
    NTSTATUS      (WINAPI *NtCreateSection)( HANDLE *handle, ACCESS_MASK access,
                                             const OBJECT_ATTRIBUTES *attr, const LARGE_INTEGER *size,
                                             ULONG protect, ULONG sec_flags, HANDLE file );
    TEB *         (WINAPI *NtCurrentTeb)(void);
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
    NTSTATUS      (WINAPI *NtLockVirtualMemory)( HANDLE process, PVOID *addr, SIZE_T *size, ULONG unknown );
    NTSTATUS      (WINAPI *NtMapViewOfSection)( HANDLE handle, HANDLE process, PVOID *addr_ptr,
                                                ULONG_PTR zero_bits, SIZE_T commit_size,
                                                const LARGE_INTEGER *offset_ptr, SIZE_T *size_ptr,
                                                SECTION_INHERIT inherit, ULONG alloc_type, ULONG protect );
    NTSTATUS      (WINAPI *NtNotifyChangeDirectoryFile)( HANDLE handle, HANDLE event, PIO_APC_ROUTINE apc,
                                                         void *apc_context, IO_STATUS_BLOCK *iosb,
                                                         void *buffer, ULONG buffer_size,
                                                         ULONG filter, BOOLEAN subtree );
    NTSTATUS      (WINAPI *NtOpenSection)( HANDLE *handle, ACCESS_MASK access,
                                           const OBJECT_ATTRIBUTES *attr );
    NTSTATUS      (WINAPI *NtPowerInformation)( POWER_INFORMATION_LEVEL level, void *input, ULONG in_size,
                                                void *output, ULONG out_size );
    NTSTATUS      (WINAPI *NtProtectVirtualMemory)( HANDLE process, PVOID *addr_ptr, SIZE_T *size_ptr,
                                                    ULONG new_prot, ULONG *old_prot );
    NTSTATUS      (WINAPI *NtQueryAttributesFile)( const OBJECT_ATTRIBUTES *attr,
                                                   FILE_BASIC_INFORMATION *info );
    NTSTATUS      (WINAPI *NtQueryDirectoryFile)( HANDLE handle, HANDLE event, PIO_APC_ROUTINE apc_routine,
                                                  void *apc_context, IO_STATUS_BLOCK *io, void *buffer,
                                                  ULONG length, FILE_INFORMATION_CLASS info_class,
                                                  BOOLEAN single_entry, UNICODE_STRING *mask,
                                                  BOOLEAN restart_scan );
    NTSTATUS      (WINAPI *NtQueryFullAttributesFile)( const OBJECT_ATTRIBUTES *attr,
                                                       FILE_NETWORK_OPEN_INFORMATION *info );
    NTSTATUS      (WINAPI *NtQueryInformationFile)( HANDLE hFile, IO_STATUS_BLOCK *io,
                                                    void *ptr, LONG len, FILE_INFORMATION_CLASS class );
    NTSTATUS      (WINAPI *NtQueryObject)( HANDLE handle, OBJECT_INFORMATION_CLASS info_class,
                                           void *ptr, ULONG len, ULONG *used_len );
    NTSTATUS      (WINAPI *NtQueryPerformanceCounter)( LARGE_INTEGER *counter, LARGE_INTEGER *frequency );
    NTSTATUS      (WINAPI *NtQuerySection)( HANDLE handle, SECTION_INFORMATION_CLASS class,
                                            void *ptr, SIZE_T size, SIZE_T *ret_size );
    NTSTATUS      (WINAPI *NtQuerySystemInformation)( SYSTEM_INFORMATION_CLASS class,
                                                      void *info, ULONG size, ULONG *ret_size );
    NTSTATUS      (WINAPI *NtQuerySystemInformationEx)( SYSTEM_INFORMATION_CLASS class,
                                                        void *query, ULONG query_len,
                                                        void *info, ULONG size, ULONG *ret_size );
    NTSTATUS      (WINAPI *NtQuerySystemTime)( LARGE_INTEGER *time );
    NTSTATUS      (WINAPI *NtQueryVirtualMemory)( HANDLE process, LPCVOID addr,
                                                  MEMORY_INFORMATION_CLASS info_class,
                                                  PVOID buffer, SIZE_T len, SIZE_T *res_len );
    NTSTATUS      (WINAPI *NtQueryVolumeInformationFile)( HANDLE handle, IO_STATUS_BLOCK *io,
                                                          void *buffer, ULONG length,
                                                          FS_INFORMATION_CLASS info_class );
    NTSTATUS      (WINAPI *NtReadFile)( HANDLE handle, HANDLE event, PIO_APC_ROUTINE apc, void *apc_user,
                                        IO_STATUS_BLOCK *io, void *buffer, ULONG length,
                                        LARGE_INTEGER *offset, ULONG *key );
    NTSTATUS      (WINAPI *NtReadFileScatter)( HANDLE file, HANDLE event, PIO_APC_ROUTINE apc,
                                               void *apc_user, IO_STATUS_BLOCK *io,
                                               FILE_SEGMENT_ELEMENT *segments, ULONG length,
                                               LARGE_INTEGER *offset, ULONG *key );
    NTSTATUS      (WINAPI *NtReadVirtualMemory)( HANDLE process, const void *addr, void *buffer,
                                                 SIZE_T size, SIZE_T *bytes_read );
    NTSTATUS      (WINAPI *NtResetWriteWatch)( HANDLE process, PVOID base, SIZE_T size );
    NTSTATUS      (WINAPI *NtSetInformationFile)( HANDLE handle, IO_STATUS_BLOCK *io,
                                                  void *ptr, ULONG len, FILE_INFORMATION_CLASS class );
    NTSTATUS      (WINAPI *NtSetInformationObject)( HANDLE handle, OBJECT_INFORMATION_CLASS info_class,
                                                    void *ptr, ULONG len );
    NTSTATUS      (WINAPI *NtSetSystemTime)( const LARGE_INTEGER *new, LARGE_INTEGER *old );
    NTSTATUS      (WINAPI *NtSetVolumeInformationFile)( HANDLE handle, IO_STATUS_BLOCK *io, void *info,
                                                        ULONG length, FS_INFORMATION_CLASS class );
    NTSTATUS      (WINAPI *NtUnlockVirtualMemory)( HANDLE process, PVOID *addr,
                                                   SIZE_T *size, ULONG unknown );
    NTSTATUS      (WINAPI *NtUnmapViewOfSection)( HANDLE process, PVOID addr );
    NTSTATUS      (WINAPI *NtWriteFile)( HANDLE handle, HANDLE event, PIO_APC_ROUTINE apc, void *apc_user,
                                         IO_STATUS_BLOCK *io, const void *buffer, ULONG length,
                                         LARGE_INTEGER *offset, ULONG *key );
    NTSTATUS      (WINAPI *NtWriteFileGather)( HANDLE file, HANDLE event, PIO_APC_ROUTINE apc,
                                               void *apc_user, IO_STATUS_BLOCK *io,
                                               FILE_SEGMENT_ELEMENT *segments, ULONG length,
                                               LARGE_INTEGER *offset, ULONG *key );
    NTSTATUS      (WINAPI *NtWriteVirtualMemory)( HANDLE process, void *addr, const void *buffer,
                                                  SIZE_T size, SIZE_T *bytes_written );

    /* other Win32 API functions */
    NTSTATUS      (WINAPI *DbgUiIssueRemoteBreakin)( HANDLE process );
    LONGLONG      (WINAPI *RtlGetSystemTimePrecise)(void);
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

    /* math functions */
    double        (CDECL *atan)( double d );
    double        (CDECL *ceil)( double d );
    double        (CDECL *cos)( double d );
    double        (CDECL *fabs)( double d );
    double        (CDECL *floor)( double d );
    double        (CDECL *log)( double d );
    double        (CDECL *pow)( double x, double y );
    double        (CDECL *sin)( double d );
    double        (CDECL *sqrt)( double d );
    double        (CDECL *tan)( double d );

    /* environment functions */
    NTSTATUS      (CDECL *get_initial_environment)( WCHAR **wargv[], WCHAR *env, SIZE_T *size );
    NTSTATUS      (CDECL *get_startup_info)( startup_info_t *info, SIZE_T *total_size, SIZE_T *info_size );
    NTSTATUS      (CDECL *get_dynamic_environment)( WCHAR *env, SIZE_T *size );
    void          (CDECL *get_initial_console)( HANDLE *handle, HANDLE *std_in,
                                                HANDLE *std_out, HANDLE *std_err );
    void          (CDECL *get_initial_directory)( UNICODE_STRING *dir );
    USHORT *      (CDECL *get_unix_codepage_data)(void);
    void          (CDECL *get_locales)( WCHAR *sys, WCHAR *user );
    const char *  (CDECL *get_version)(void);
    const char *  (CDECL *get_build_id)(void);
    void          (CDECL *get_host_version)( const char **sysname, const char **release );

    /* virtual memory functions */
    NTSTATUS      (CDECL *virtual_map_section)( HANDLE handle, PVOID *addr_ptr, unsigned short zero_bits_64, SIZE_T commit_size,
                                                const LARGE_INTEGER *offset_ptr, SIZE_T *size_ptr, ULONG alloc_type,
                                                ULONG protect, pe_image_info_t *image_info );
    NTSTATUS      (CDECL *virtual_alloc_thread_stack)( INITIAL_TEB *stack, SIZE_T reserve_size, SIZE_T commit_size, SIZE_T *pthread_size );
    ssize_t       (CDECL *virtual_locked_recvmsg)( int fd, struct msghdr *hdr, int flags );
    void          (CDECL *virtual_release_address_space)(void);
    void          (CDECL *virtual_set_large_address_space)(void);

    /* thread/process functions */
    void          (CDECL *exit_thread)( int status );
    void          (CDECL *exit_process)( int status );
    NTSTATUS      (CDECL *exec_process)( UNICODE_STRING *path, UNICODE_STRING *cmdline, NTSTATUS status );

    /* server functions */
    unsigned int  (CDECL *server_call)( void *req_ptr );
    void          (CDECL *server_send_fd)( int fd );
    NTSTATUS      (CDECL *server_fd_to_handle)( int fd, unsigned int access, unsigned int attributes,
                                                HANDLE *handle );
    NTSTATUS      (CDECL *server_handle_to_fd)( HANDLE handle, unsigned int access, int *unix_fd,
                                                unsigned int *options );
    void          (CDECL *server_release_fd)( HANDLE handle, int unix_fd );
    void          (CDECL *server_init_process_done)( void *relay );

    /* file functions */
    NTSTATUS      (CDECL *nt_to_unix_file_name)( const UNICODE_STRING *nameW, char *nameA, SIZE_T *size,
                                                 UINT disposition );
    NTSTATUS      (CDECL *unix_to_nt_file_name)( const char *name, WCHAR *buffer, SIZE_T *size );
    void          (CDECL *set_show_dot_files)( BOOL enable );

    /* loader functions */
    NTSTATUS      (CDECL *load_so_dll)( UNICODE_STRING *nt_name, void **module );
    NTSTATUS      (CDECL *load_builtin_dll)( const WCHAR *name, void **module,
                                             pe_image_info_t *image_info );
    NTSTATUS      (CDECL *unload_builtin_dll)( void *module );
    void          (CDECL *init_builtin_dll)( void *module );
    NTSTATUS      (CDECL *unwind_builtin_dll)( ULONG type, struct _DISPATCHER_CONTEXT *dispatch,
                                               CONTEXT *context );

    /* debugging functions */
    unsigned char (CDECL *dbg_get_channel_flags)( struct __wine_debug_channel *channel );
    const char *  (CDECL *dbg_strdup)( const char *str );
    int           (CDECL *dbg_output)( const char *str );
    int           (CDECL *dbg_header)( enum __wine_debug_class cls, struct __wine_debug_channel *channel,
                                       const char *function );
};

#endif /* __NTDLL_UNIXLIB_H */
