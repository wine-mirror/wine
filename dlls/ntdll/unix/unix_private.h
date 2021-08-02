/*
 * Ntdll Unix private interface
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

#ifndef __NTDLL_UNIX_PRIVATE_H
#define __NTDLL_UNIX_PRIVATE_H

#include <pthread.h>
#include <signal.h>
#include "unixlib.h"
#include "wine/server.h"
#include "wine/list.h"

struct msghdr;

#ifdef __i386__
static const WORD current_machine = IMAGE_FILE_MACHINE_I386;
#elif defined(__x86_64__)
static const WORD current_machine = IMAGE_FILE_MACHINE_AMD64;
#elif defined(__arm__)
static const WORD current_machine = IMAGE_FILE_MACHINE_ARMNT;
#elif defined(__aarch64__)
static const WORD current_machine = IMAGE_FILE_MACHINE_ARM64;
#endif
extern WORD native_machine DECLSPEC_HIDDEN;

static const BOOL is_win64 = (sizeof(void *) > sizeof(int));

static inline BOOL is_machine_64bit( WORD machine )
{
    return (machine == IMAGE_FILE_MACHINE_AMD64 || machine == IMAGE_FILE_MACHINE_ARM64);
}

/* thread private data, stored in NtCurrentTeb()->GdiTebBatch */
struct ntdll_thread_data
{
    void              *cpu_data[16];  /* reserved for CPU-specific data */
    void              *kernel_stack;  /* stack for thread startup and kernel syscalls */
    int                request_fd;    /* fd for sending server requests */
    int                reply_fd;      /* fd for receiving server replies */
    int                wait_fd[2];    /* fd for sleeping server requests */
    pthread_t          pthread_id;    /* pthread thread id */
    struct list        entry;         /* entry in TEB list */
    PRTL_THREAD_START_ROUTINE start;  /* thread entry point */
    void              *param;         /* thread entry point parameter */
    void              *jmp_buf;       /* setjmp buffer for exception handling */
};

C_ASSERT( sizeof(struct ntdll_thread_data) <= sizeof(((TEB *)0)->GdiTebBatch) );

static inline struct ntdll_thread_data *ntdll_get_thread_data(void)
{
    return (struct ntdll_thread_data *)&NtCurrentTeb()->GdiTebBatch;
}

typedef NTSTATUS async_callback_t( void *user, ULONG_PTR *info, NTSTATUS status );

struct async_fileio
{
    async_callback_t    *callback;
    struct async_fileio *next;
    HANDLE               handle;
};

static const SIZE_T page_size = 0x1000;
static const SIZE_T teb_size = 0x3800;  /* TEB64 + TEB32 + debug info */
static const SIZE_T signal_stack_mask = 0xffff;
static const SIZE_T signal_stack_size = 0x10000 - 0x3800;
static const SIZE_T kernel_stack_size = 0x20000;
static const LONG teb_offset = 0x2000;

/* callbacks to PE ntdll from the Unix side */
extern void     (WINAPI *pDbgUiRemoteBreakin)( void *arg ) DECLSPEC_HIDDEN;
extern NTSTATUS (WINAPI *pKiRaiseUserExceptionDispatcher)(void) DECLSPEC_HIDDEN;
extern void     (WINAPI *pKiUserApcDispatcher)(CONTEXT*,ULONG_PTR,ULONG_PTR,ULONG_PTR,PNTAPCFUNC) DECLSPEC_HIDDEN;
extern NTSTATUS (WINAPI *pKiUserExceptionDispatcher)(EXCEPTION_RECORD*,CONTEXT*) DECLSPEC_HIDDEN;
extern void     (WINAPI *pLdrInitializeThunk)(CONTEXT*,void**,ULONG_PTR,ULONG_PTR) DECLSPEC_HIDDEN;
extern void     (WINAPI *pRtlUserThreadStart)( PRTL_THREAD_START_ROUTINE entry, void *arg ) DECLSPEC_HIDDEN;
extern void     (WINAPI *p__wine_ctrl_routine)(void *) DECLSPEC_HIDDEN;
extern SYSTEM_DLL_INIT_BLOCK *pLdrSystemDllInitBlock DECLSPEC_HIDDEN;

extern NTSTATUS CDECL fast_RtlpWaitForCriticalSection( RTL_CRITICAL_SECTION *crit, int timeout ) DECLSPEC_HIDDEN;
extern NTSTATUS CDECL fast_RtlpUnWaitCriticalSection( RTL_CRITICAL_SECTION *crit ) DECLSPEC_HIDDEN;
extern NTSTATUS CDECL fast_RtlDeleteCriticalSection( RTL_CRITICAL_SECTION *crit ) DECLSPEC_HIDDEN;
extern NTSTATUS CDECL fast_RtlTryAcquireSRWLockExclusive( RTL_SRWLOCK *lock ) DECLSPEC_HIDDEN;
extern NTSTATUS CDECL fast_RtlAcquireSRWLockExclusive( RTL_SRWLOCK *lock ) DECLSPEC_HIDDEN;
extern NTSTATUS CDECL fast_RtlTryAcquireSRWLockShared( RTL_SRWLOCK *lock ) DECLSPEC_HIDDEN;
extern NTSTATUS CDECL fast_RtlAcquireSRWLockShared( RTL_SRWLOCK *lock ) DECLSPEC_HIDDEN;
extern NTSTATUS CDECL fast_RtlReleaseSRWLockExclusive( RTL_SRWLOCK *lock ) DECLSPEC_HIDDEN;
extern NTSTATUS CDECL fast_RtlReleaseSRWLockShared( RTL_SRWLOCK *lock ) DECLSPEC_HIDDEN;
extern NTSTATUS CDECL fast_RtlWakeConditionVariable( RTL_CONDITION_VARIABLE *variable, int count ) DECLSPEC_HIDDEN;
extern LONGLONG CDECL fast_RtlGetSystemTimePrecise(void) DECLSPEC_HIDDEN;
extern NTSTATUS CDECL fast_wait_cv( RTL_CONDITION_VARIABLE *variable, const void *value,
                                    const LARGE_INTEGER *timeout ) DECLSPEC_HIDDEN;

extern void CDECL virtual_release_address_space(void) DECLSPEC_HIDDEN;

extern NTSTATUS CDECL unwind_builtin_dll( ULONG type, struct _DISPATCHER_CONTEXT *dispatch,
                                          CONTEXT *context ) DECLSPEC_HIDDEN;

extern const char *home_dir DECLSPEC_HIDDEN;
extern const char *data_dir DECLSPEC_HIDDEN;
extern const char *build_dir DECLSPEC_HIDDEN;
extern const char *config_dir DECLSPEC_HIDDEN;
extern const char *user_name DECLSPEC_HIDDEN;
extern const char **dll_paths DECLSPEC_HIDDEN;
extern PEB *peb DECLSPEC_HIDDEN;
extern USHORT *uctable DECLSPEC_HIDDEN;
extern USHORT *lctable DECLSPEC_HIDDEN;
extern SIZE_T startup_info_size DECLSPEC_HIDDEN;
extern BOOL is_prefix_bootstrap DECLSPEC_HIDDEN;
extern SECTION_IMAGE_INFORMATION main_image_info DECLSPEC_HIDDEN;
extern int main_argc DECLSPEC_HIDDEN;
extern char **main_argv DECLSPEC_HIDDEN;
extern char **main_envp DECLSPEC_HIDDEN;
extern WCHAR **main_wargv DECLSPEC_HIDDEN;
extern const WCHAR system_dir[] DECLSPEC_HIDDEN;
extern unsigned int supported_machines_count DECLSPEC_HIDDEN;
extern USHORT supported_machines[8] DECLSPEC_HIDDEN;
extern BOOL process_exiting DECLSPEC_HIDDEN;
extern HANDLE keyed_event DECLSPEC_HIDDEN;
extern timeout_t server_start_time DECLSPEC_HIDDEN;
extern sigset_t server_block_set DECLSPEC_HIDDEN;
extern struct _KUSER_SHARED_DATA *user_shared_data DECLSPEC_HIDDEN;
extern SYSTEM_CPU_INFORMATION cpu_info DECLSPEC_HIDDEN;
#ifndef _WIN64
extern BOOL is_wow64 DECLSPEC_HIDDEN;
#endif
#ifdef __i386__
extern struct ldt_copy __wine_ldt_copy DECLSPEC_HIDDEN;
#endif

extern void init_environment( int argc, char *argv[], char *envp[] ) DECLSPEC_HIDDEN;
extern void init_startup_info(void) DECLSPEC_HIDDEN;
extern void *create_startup_info( const UNICODE_STRING *nt_image, const RTL_USER_PROCESS_PARAMETERS *params,
                                  DWORD *info_size ) DECLSPEC_HIDDEN;
extern DWORD ntdll_umbstowcs( const char *src, DWORD srclen, WCHAR *dst, DWORD dstlen ) DECLSPEC_HIDDEN;
extern int ntdll_wcstoumbs( const WCHAR *src, DWORD srclen, char *dst, DWORD dstlen, BOOL strict ) DECLSPEC_HIDDEN;
extern char **build_envp( const WCHAR *envW ) DECLSPEC_HIDDEN;
extern NTSTATUS exec_wineloader( char **argv, int socketfd, const pe_image_info_t *pe_info ) DECLSPEC_HIDDEN;
extern NTSTATUS load_builtin( const pe_image_info_t *image_info, WCHAR *filename,
                              void **addr_ptr, SIZE_T *size_ptr ) DECLSPEC_HIDDEN;
extern BOOL is_builtin_path( const UNICODE_STRING *path, WORD *machine ) DECLSPEC_HIDDEN;
extern NTSTATUS load_main_exe( const WCHAR *name, const char *unix_name, const WCHAR *curdir, WCHAR **image,
                               void **module ) DECLSPEC_HIDDEN;
extern NTSTATUS load_start_exe( WCHAR **image, void **module ) DECLSPEC_HIDDEN;
extern void start_server( BOOL debug ) DECLSPEC_HIDDEN;

extern unsigned int server_call_unlocked( void *req_ptr ) DECLSPEC_HIDDEN;
extern void server_enter_uninterrupted_section( pthread_mutex_t *mutex, sigset_t *sigset ) DECLSPEC_HIDDEN;
extern void server_leave_uninterrupted_section( pthread_mutex_t *mutex, sigset_t *sigset ) DECLSPEC_HIDDEN;
extern unsigned int server_select( const select_op_t *select_op, data_size_t size, UINT flags,
                                   timeout_t abs_timeout, context_t *context, pthread_mutex_t *mutex,
                                   user_apc_t *user_apc ) DECLSPEC_HIDDEN;
extern unsigned int server_wait( const select_op_t *select_op, data_size_t size, UINT flags,
                                 const LARGE_INTEGER *timeout ) DECLSPEC_HIDDEN;
extern unsigned int server_queue_process_apc( HANDLE process, const apc_call_t *call,
                                              apc_result_t *result ) DECLSPEC_HIDDEN;
extern int server_get_unix_fd( HANDLE handle, unsigned int wanted_access, int *unix_fd,
                               int *needs_close, enum server_fd_type *type, unsigned int *options ) DECLSPEC_HIDDEN;
extern void wine_server_send_fd( int fd ) DECLSPEC_HIDDEN;
extern void process_exit_wrapper( int status ) DECLSPEC_HIDDEN;
extern size_t server_init_process(void) DECLSPEC_HIDDEN;
extern void server_init_process_done(void) DECLSPEC_HIDDEN;
extern void server_init_thread( void *entry_point, BOOL *suspend ) DECLSPEC_HIDDEN;
extern int server_pipe( int fd[2] ) DECLSPEC_HIDDEN;

extern void fpux_to_fpu( I386_FLOATING_SAVE_AREA *fpu, const XSAVE_FORMAT *fpux ) DECLSPEC_HIDDEN;
extern void fpu_to_fpux( XSAVE_FORMAT *fpux, const I386_FLOATING_SAVE_AREA *fpu ) DECLSPEC_HIDDEN;
extern void *get_cpu_area( USHORT machine ) DECLSPEC_HIDDEN;
extern void set_thread_id( TEB *teb, DWORD pid, DWORD tid ) DECLSPEC_HIDDEN;
extern NTSTATUS init_thread_stack( TEB *teb, ULONG_PTR zero_bits, SIZE_T reserve_size, SIZE_T commit_size ) DECLSPEC_HIDDEN;
extern void DECLSPEC_NORETURN abort_thread( int status ) DECLSPEC_HIDDEN;
extern void DECLSPEC_NORETURN abort_process( int status ) DECLSPEC_HIDDEN;
extern void DECLSPEC_NORETURN exit_process( int status ) DECLSPEC_HIDDEN;
extern void wait_suspend( CONTEXT *context ) DECLSPEC_HIDDEN;
extern NTSTATUS send_debug_event( EXCEPTION_RECORD *rec, CONTEXT *context, BOOL first_chance ) DECLSPEC_HIDDEN;
extern NTSTATUS set_thread_context( HANDLE handle, const void *context, BOOL *self, USHORT machine ) DECLSPEC_HIDDEN;
extern NTSTATUS get_thread_context( HANDLE handle, void *context, BOOL *self, USHORT machine ) DECLSPEC_HIDDEN;
extern NTSTATUS alloc_object_attributes( const OBJECT_ATTRIBUTES *attr, struct object_attributes **ret,
                                         data_size_t *ret_len ) DECLSPEC_HIDDEN;

extern void *anon_mmap_fixed( void *start, size_t size, int prot, int flags ) DECLSPEC_HIDDEN;
extern void *anon_mmap_alloc( size_t size, int prot ) DECLSPEC_HIDDEN;
extern void virtual_init(void) DECLSPEC_HIDDEN;
extern ULONG_PTR get_system_affinity_mask(void) DECLSPEC_HIDDEN;
extern void virtual_get_system_info( SYSTEM_BASIC_INFORMATION *info, BOOL wow64 ) DECLSPEC_HIDDEN;
extern NTSTATUS virtual_map_builtin_module( HANDLE mapping, void **module, SIZE_T *size,
                                            SECTION_IMAGE_INFORMATION *info, WORD machine, BOOL prefer_native ) DECLSPEC_HIDDEN;
extern NTSTATUS virtual_create_builtin_view( void *module, const UNICODE_STRING *nt_name,
                                             pe_image_info_t *info, void *so_handle ) DECLSPEC_HIDDEN;
extern TEB *virtual_alloc_first_teb(void) DECLSPEC_HIDDEN;
extern NTSTATUS virtual_alloc_teb( TEB **ret_teb ) DECLSPEC_HIDDEN;
extern void virtual_free_teb( TEB *teb ) DECLSPEC_HIDDEN;
extern NTSTATUS virtual_clear_tls_index( ULONG index ) DECLSPEC_HIDDEN;
extern NTSTATUS virtual_alloc_thread_stack( INITIAL_TEB *stack, ULONG_PTR zero_bits, SIZE_T reserve_size,
                                            SIZE_T commit_size, SIZE_T extra_size ) DECLSPEC_HIDDEN;
extern void virtual_map_user_shared_data(void) DECLSPEC_HIDDEN;
extern NTSTATUS virtual_handle_fault( void *addr, DWORD err, void *stack ) DECLSPEC_HIDDEN;
extern unsigned int virtual_locked_server_call( void *req_ptr ) DECLSPEC_HIDDEN;
extern ssize_t virtual_locked_read( int fd, void *addr, size_t size ) DECLSPEC_HIDDEN;
extern ssize_t virtual_locked_pread( int fd, void *addr, size_t size, off_t offset ) DECLSPEC_HIDDEN;
extern ssize_t virtual_locked_recvmsg( int fd, struct msghdr *hdr, int flags ) DECLSPEC_HIDDEN;
extern BOOL virtual_is_valid_code_address( const void *addr, SIZE_T size ) DECLSPEC_HIDDEN;
extern void *virtual_setup_exception( void *stack_ptr, size_t size, EXCEPTION_RECORD *rec ) DECLSPEC_HIDDEN;
extern BOOL virtual_check_buffer_for_read( const void *ptr, SIZE_T size ) DECLSPEC_HIDDEN;
extern BOOL virtual_check_buffer_for_write( void *ptr, SIZE_T size ) DECLSPEC_HIDDEN;
extern SIZE_T virtual_uninterrupted_read_memory( const void *addr, void *buffer, SIZE_T size ) DECLSPEC_HIDDEN;
extern NTSTATUS virtual_uninterrupted_write_memory( void *addr, const void *buffer, SIZE_T size ) DECLSPEC_HIDDEN;
extern void virtual_set_force_exec( BOOL enable ) DECLSPEC_HIDDEN;
extern void virtual_set_large_address_space(void) DECLSPEC_HIDDEN;
extern void virtual_fill_image_information( const pe_image_info_t *pe_info,
                                            SECTION_IMAGE_INFORMATION *info ) DECLSPEC_HIDDEN;
extern NTSTATUS release_builtin_module( void *module ) DECLSPEC_HIDDEN;
extern void *get_builtin_so_handle( void *module ) DECLSPEC_HIDDEN;
extern NTSTATUS get_builtin_unix_info( void *module, const char **name, void **handle, void **entry ) DECLSPEC_HIDDEN;
extern NTSTATUS set_builtin_unix_info( void *module, const char *name, void *handle, void *entry ) DECLSPEC_HIDDEN;

extern NTSTATUS get_thread_ldt_entry( HANDLE handle, void *data, ULONG len, ULONG *ret_len ) DECLSPEC_HIDDEN;
extern void *get_native_context( CONTEXT *context ) DECLSPEC_HIDDEN;
extern void *get_wow_context( CONTEXT *context ) DECLSPEC_HIDDEN;
extern BOOL get_thread_times( int unix_pid, int unix_tid, LARGE_INTEGER *kernel_time,
                              LARGE_INTEGER *user_time ) DECLSPEC_HIDDEN;
extern void signal_init_threading(void) DECLSPEC_HIDDEN;
extern NTSTATUS signal_alloc_thread( TEB *teb ) DECLSPEC_HIDDEN;
extern void signal_free_thread( TEB *teb ) DECLSPEC_HIDDEN;
extern void signal_init_thread( TEB *teb ) DECLSPEC_HIDDEN;
extern void signal_init_process(void) DECLSPEC_HIDDEN;
extern void DECLSPEC_NORETURN signal_start_thread( PRTL_THREAD_START_ROUTINE entry, void *arg,
                                                   BOOL suspend, TEB *teb ) DECLSPEC_HIDDEN;
extern void DECLSPEC_NORETURN signal_exit_thread( int status, void (*func)(int), TEB *teb ) DECLSPEC_HIDDEN;
extern void __wine_syscall_dispatcher(void) DECLSPEC_HIDDEN;
extern void WINAPI DECLSPEC_NORETURN __wine_syscall_dispatcher_return( void *frame, ULONG_PTR retval ) DECLSPEC_HIDDEN;
extern unsigned int __wine_syscall_flags DECLSPEC_HIDDEN;
extern NTSTATUS signal_set_full_context( CONTEXT *context ) DECLSPEC_HIDDEN;
extern NTSTATUS get_thread_wow64_context( HANDLE handle, void *ctx, ULONG size ) DECLSPEC_HIDDEN;
extern NTSTATUS set_thread_wow64_context( HANDLE handle, const void *ctx, ULONG size ) DECLSPEC_HIDDEN;
extern void fill_vm_counters( VM_COUNTERS_EX *pvmi, int unix_pid ) DECLSPEC_HIDDEN;
extern NTSTATUS open_hkcu_key( const char *path, HANDLE *key ) DECLSPEC_HIDDEN;

extern NTSTATUS cdrom_DeviceIoControl( HANDLE device, HANDLE event, PIO_APC_ROUTINE apc, void *apc_user,
                                       IO_STATUS_BLOCK *io, ULONG code, void *in_buffer,
                                       ULONG in_size, void *out_buffer, ULONG out_size ) DECLSPEC_HIDDEN;
extern NTSTATUS serial_DeviceIoControl( HANDLE device, HANDLE event, PIO_APC_ROUTINE apc, void *apc_user,
                                        IO_STATUS_BLOCK *io, ULONG code, void *in_buffer,
                                        ULONG in_size, void *out_buffer, ULONG out_size ) DECLSPEC_HIDDEN;
extern NTSTATUS serial_FlushBuffersFile( int fd ) DECLSPEC_HIDDEN;
extern NTSTATUS sock_ioctl( HANDLE handle, HANDLE event, PIO_APC_ROUTINE apc, void *apc_user, IO_STATUS_BLOCK *io,
                            ULONG code, void *in_buffer, ULONG in_size, void *out_buffer, ULONG out_size ) DECLSPEC_HIDDEN;
extern NTSTATUS tape_DeviceIoControl( HANDLE device, HANDLE event, PIO_APC_ROUTINE apc, void *apc_user,
                                      IO_STATUS_BLOCK *io, ULONG code, void *in_buffer,
                                      ULONG in_size, void *out_buffer, ULONG out_size ) DECLSPEC_HIDDEN;

extern struct async_fileio *alloc_fileio( DWORD size, async_callback_t callback, HANDLE handle ) DECLSPEC_HIDDEN;
extern void release_fileio( struct async_fileio *io ) DECLSPEC_HIDDEN;
extern NTSTATUS errno_to_status( int err ) DECLSPEC_HIDDEN;
extern BOOL get_redirect( OBJECT_ATTRIBUTES *attr, UNICODE_STRING *redir ) DECLSPEC_HIDDEN;
extern NTSTATUS nt_to_unix_file_name( const OBJECT_ATTRIBUTES *attr, char **name_ret, UINT disposition ) DECLSPEC_HIDDEN;
extern NTSTATUS unix_to_nt_file_name( const char *name, WCHAR **nt ) DECLSPEC_HIDDEN;
extern NTSTATUS get_full_path( const WCHAR *name, const WCHAR *curdir, WCHAR **path ) DECLSPEC_HIDDEN;
extern NTSTATUS open_unix_file( HANDLE *handle, const char *unix_name, ACCESS_MASK access,
                                OBJECT_ATTRIBUTES *attr, ULONG attributes, ULONG sharing, ULONG disposition,
                                ULONG options, void *ea_buffer, ULONG ea_length ) DECLSPEC_HIDDEN;
extern void init_files(void) DECLSPEC_HIDDEN;
extern void init_cpu_info(void) DECLSPEC_HIDDEN;
extern void add_completion( HANDLE handle, ULONG_PTR value, NTSTATUS status, ULONG info, BOOL async ) DECLSPEC_HIDDEN;

extern void dbg_init(void) DECLSPEC_HIDDEN;

extern NTSTATUS call_user_apc_dispatcher( CONTEXT *context_ptr, ULONG_PTR arg1, ULONG_PTR arg2, ULONG_PTR arg3,
                                          PNTAPCFUNC func, NTSTATUS status ) DECLSPEC_HIDDEN;
extern NTSTATUS call_user_exception_dispatcher( EXCEPTION_RECORD *rec, CONTEXT *context ) DECLSPEC_HIDDEN;
extern void call_raise_user_exception_dispatcher(void) DECLSPEC_HIDDEN;

#define IMAGE_DLLCHARACTERISTICS_PREFER_NATIVE 0x0010 /* Wine extension */

#define TICKSPERSEC 10000000
#define SECS_1601_TO_1970  ((369 * 365 + 89) * (ULONGLONG)86400)

static inline ULONGLONG ticks_from_time_t( time_t time )
{
    if (sizeof(time_t) == sizeof(int))  /* time_t may be signed */
        return ((ULONGLONG)(ULONG)time + SECS_1601_TO_1970) * TICKSPERSEC;
    else
        return ((ULONGLONG)time + SECS_1601_TO_1970) * TICKSPERSEC;
}

static inline const char *debugstr_us( const UNICODE_STRING *us )
{
    if (!us) return "<null>";
    return debugstr_wn( us->Buffer, us->Length / sizeof(WCHAR) );
}

/* convert from straight ASCII to Unicode without depending on the current codepage */
static inline void ascii_to_unicode( WCHAR *dst, const char *src, size_t len )
{
    while (len--) *dst++ = (unsigned char)*src++;
}

static inline void *get_signal_stack(void)
{
    return (void *)(((ULONG_PTR)NtCurrentTeb() & ~signal_stack_mask) + teb_size);
}

static inline BOOL is_inside_signal_stack( void *ptr )
{
    return ((char *)ptr >= (char *)get_signal_stack() &&
            (char *)ptr < (char *)get_signal_stack() + signal_stack_size);
}

static inline void mutex_lock( pthread_mutex_t *mutex )
{
    if (!process_exiting) pthread_mutex_lock( mutex );
}

static inline void mutex_unlock( pthread_mutex_t *mutex )
{
    if (!process_exiting) pthread_mutex_unlock( mutex );
}

#ifdef _WIN64
typedef TEB32 WOW_TEB;
static inline TEB64 *NtCurrentTeb64(void) { return NULL; }
#else
typedef TEB64 WOW_TEB;
static inline TEB64 *NtCurrentTeb64(void) { return (TEB64 *)NtCurrentTeb()->GdiBatchCount; }
#endif

static inline WOW_TEB *get_wow_teb( TEB *teb )
{
    return teb->WowTebOffset ? (WOW_TEB *)((char *)teb + teb->WowTebOffset) : NULL;
}

enum loadorder
{
    LO_INVALID,
    LO_DISABLED,
    LO_NATIVE,
    LO_BUILTIN,
    LO_NATIVE_BUILTIN,  /* native then builtin */
    LO_BUILTIN_NATIVE,  /* builtin then native */
    LO_DEFAULT          /* nothing specified, use default strategy */
};

extern void set_load_order_app_name( const WCHAR *app_name ) DECLSPEC_HIDDEN;
extern enum loadorder get_load_order( const UNICODE_STRING *nt_name ) DECLSPEC_HIDDEN;

static inline size_t ntdll_wcslen( const WCHAR *str )
{
    const WCHAR *s = str;
    while (*s) s++;
    return s - str;
}

static inline WCHAR *ntdll_wcscpy( WCHAR *dst, const WCHAR *src )
{
    WCHAR *p = dst;
    while ((*p++ = *src++));
    return dst;
}

static inline WCHAR *ntdll_wcscat( WCHAR *dst, const WCHAR *src )
{
    ntdll_wcscpy( dst + ntdll_wcslen(dst), src );
    return dst;
}

static inline int ntdll_wcscmp( const WCHAR *str1, const WCHAR *str2 )
{
    while (*str1 && (*str1 == *str2)) { str1++; str2++; }
    return *str1 - *str2;
}

static inline int ntdll_wcsncmp( const WCHAR *str1, const WCHAR *str2, int n )
{
    if (n <= 0) return 0;
    while ((--n > 0) && *str1 && (*str1 == *str2)) { str1++; str2++; }
    return *str1 - *str2;
}

static inline WCHAR *ntdll_wcschr( const WCHAR *str, WCHAR ch )
{
    do { if (*str == ch) return (WCHAR *)(ULONG_PTR)str; } while (*str++);
    return NULL;
}

static inline WCHAR *ntdll_wcsrchr( const WCHAR *str, WCHAR ch )
{
    WCHAR *ret = NULL;
    do { if (*str == ch) ret = (WCHAR *)(ULONG_PTR)str; } while (*str++);
    return ret;
}

static inline WCHAR *ntdll_wcspbrk( const WCHAR *str, const WCHAR *accept )
{
    for ( ; *str; str++) if (ntdll_wcschr( accept, *str )) return (WCHAR *)(ULONG_PTR)str;
    return NULL;
}

static inline SIZE_T ntdll_wcsspn( const WCHAR *str, const WCHAR *accept )
{
    const WCHAR *ptr;
    for (ptr = str; *ptr; ptr++) if (!ntdll_wcschr( accept, *ptr )) break;
    return ptr - str;
}

static inline SIZE_T ntdll_wcscspn( const WCHAR *str, const WCHAR *reject )
{
    const WCHAR *ptr;
    for (ptr = str; *ptr; ptr++) if (ntdll_wcschr( reject, *ptr )) break;
    return ptr - str;
}

static inline WCHAR ntdll_towupper( WCHAR ch )
{
    return ch + uctable[uctable[uctable[ch >> 8] + ((ch >> 4) & 0x0f)] + (ch & 0x0f)];
}

static inline WCHAR ntdll_towlower( WCHAR ch )
{
    return ch + lctable[lctable[lctable[ch >> 8] + ((ch >> 4) & 0x0f)] + (ch & 0x0f)];
}

static inline WCHAR *ntdll_wcsupr( WCHAR *str )
{
    WCHAR *ret;
    for (ret = str; *str; str++) *str = ntdll_towupper(*str);
    return ret;
}

static inline int ntdll_wcsicmp( const WCHAR *str1, const WCHAR *str2 )
{
    int ret;
    for (;;)
    {
        if ((ret = ntdll_towupper( *str1 ) - ntdll_towupper( *str2 )) || !*str1) return ret;
        str1++;
        str2++;
    }
}

static inline int ntdll_wcsnicmp( const WCHAR *str1, const WCHAR *str2, int n )
{
    int ret;
    for (ret = 0; n > 0; n--, str1++, str2++)
        if ((ret = ntdll_towupper(*str1) - ntdll_towupper(*str2)) || !*str1) break;
    return ret;
}

#define wcslen(str)        ntdll_wcslen(str)
#define wcscpy(dst,src)    ntdll_wcscpy(dst,src)
#define wcscat(dst,src)    ntdll_wcscat(dst,src)
#define wcscmp(s1,s2)      ntdll_wcscmp(s1,s2)
#define wcsncmp(s1,s2,n)   ntdll_wcsncmp(s1,s2,n)
#define wcschr(str,ch)     ntdll_wcschr(str,ch)
#define wcsrchr(str,ch)    ntdll_wcsrchr(str,ch)
#define wcspbrk(str,ac)    ntdll_wcspbrk(str,ac)
#define wcsspn(str,ac)     ntdll_wcsspn(str,ac)
#define wcscspn(str,rej)   ntdll_wcscspn(str,rej)
#define wcsicmp(s1, s2)    ntdll_wcsicmp(s1,s2)
#define wcsnicmp(s1, s2,n) ntdll_wcsnicmp(s1,s2,n)
#define wcsupr(str)        ntdll_wcsupr(str)
#define towupper(c)        ntdll_towupper(c)
#define towlower(c)        ntdll_towlower(c)

static inline void init_unicode_string( UNICODE_STRING *str, const WCHAR *data )
{
    str->Length = wcslen(data) * sizeof(WCHAR);
    str->MaximumLength = str->Length + sizeof(WCHAR);
    str->Buffer = (WCHAR *)data;
}

#endif /* __NTDLL_UNIX_PRIVATE_H */
