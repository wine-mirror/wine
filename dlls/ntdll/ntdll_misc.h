/*
 * Copyright 2000 Juergen Schmied
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

#ifndef __WINE_NTDLL_MISC_H
#define __WINE_NTDLL_MISC_H

#include <stdarg.h>
#include <signal.h>
#include <sys/types.h>
#include <pthread.h>

#include "windef.h"
#include "winnt.h"
#include "winternl.h"
#include "unixlib.h"
#include "wine/server.h"
#include "wine/asm.h"

#define DECLARE_CRITICAL_SECTION(cs) \
    static RTL_CRITICAL_SECTION cs; \
    static RTL_CRITICAL_SECTION_DEBUG cs##_debug = \
    { 0, 0, &cs, { &cs##_debug.ProcessLocksList, &cs##_debug.ProcessLocksList }, \
      0, 0, { (DWORD_PTR)(__FILE__ ": " # cs) }}; \
    static RTL_CRITICAL_SECTION cs = { &cs##_debug, -1, 0, 0, 0, 0 };

#define MAX_NT_PATH_LENGTH 277

#if defined(__i386__) || defined(__x86_64__) || defined(__arm__) || defined(__aarch64__)
static const UINT_PTR page_size = 0x1000;
#else
extern UINT_PTR page_size DECLSPEC_HIDDEN;
#endif

extern NTSTATUS close_handle( HANDLE ) DECLSPEC_HIDDEN;

/* exceptions */
extern LONG call_vectored_handlers( EXCEPTION_RECORD *rec, CONTEXT *context ) DECLSPEC_HIDDEN;
extern void DECLSPEC_NORETURN raise_status( NTSTATUS status, EXCEPTION_RECORD *rec ) DECLSPEC_HIDDEN;
extern LONG WINAPI call_unhandled_exception_filter( PEXCEPTION_POINTERS eptr ) DECLSPEC_HIDDEN;

#if defined(__x86_64__) || defined(__arm__) || defined(__aarch64__)
extern RUNTIME_FUNCTION *lookup_function_info( ULONG_PTR pc, ULONG_PTR *base, LDR_DATA_TABLE_ENTRY **module ) DECLSPEC_HIDDEN;
#endif

/* debug helpers */
extern LPCSTR debugstr_us( const UNICODE_STRING *str ) DECLSPEC_HIDDEN;
extern LPCSTR debugstr_ObjectAttributes(const OBJECT_ATTRIBUTES *oa) DECLSPEC_HIDDEN;

/* init routines */
extern void version_init(void) DECLSPEC_HIDDEN;
extern void debug_init(void) DECLSPEC_HIDDEN;
extern TEB *thread_init( SIZE_T *info_size, BOOL *suspend ) DECLSPEC_HIDDEN;
extern void actctx_init(void) DECLSPEC_HIDDEN;
extern void fill_cpu_info(void) DECLSPEC_HIDDEN;
extern void heap_set_debug_flags( HANDLE handle ) DECLSPEC_HIDDEN;
extern void init_unix_codepage(void) DECLSPEC_HIDDEN;
extern void init_locale( HMODULE module ) DECLSPEC_HIDDEN;
extern void init_user_process_params( SIZE_T data_size ) DECLSPEC_HIDDEN;
extern char **build_envp( const WCHAR *envW ) DECLSPEC_HIDDEN;
extern NTSTATUS restart_process( RTL_USER_PROCESS_PARAMETERS *params, NTSTATUS status ) DECLSPEC_HIDDEN;

/* server support */
extern const char *build_dir DECLSPEC_HIDDEN;
extern const char *data_dir DECLSPEC_HIDDEN;
extern const char *config_dir DECLSPEC_HIDDEN;
extern timeout_t server_start_time DECLSPEC_HIDDEN;
extern unsigned int server_cpus DECLSPEC_HIDDEN;
extern BOOL is_wow64 DECLSPEC_HIDDEN;
extern NTSTATUS alloc_object_attributes( const OBJECT_ATTRIBUTES *attr, struct object_attributes **ret,
                                         data_size_t *ret_len ) DECLSPEC_HIDDEN;
extern NTSTATUS validate_open_object_attributes( const OBJECT_ATTRIBUTES *attr ) DECLSPEC_HIDDEN;

/* module handling */
extern LIST_ENTRY tls_links DECLSPEC_HIDDEN;
extern FARPROC RELAY_GetProcAddress( HMODULE module, const IMAGE_EXPORT_DIRECTORY *exports,
                                     DWORD exp_size, FARPROC proc, DWORD ordinal, const WCHAR *user ) DECLSPEC_HIDDEN;
extern FARPROC SNOOP_GetProcAddress( HMODULE hmod, const IMAGE_EXPORT_DIRECTORY *exports, DWORD exp_size,
                                     FARPROC origfun, DWORD ordinal, const WCHAR *user ) DECLSPEC_HIDDEN;
extern void RELAY_SetupDLL( HMODULE hmod ) DECLSPEC_HIDDEN;
extern void SNOOP_SetupDLL( HMODULE hmod ) DECLSPEC_HIDDEN;
extern const WCHAR windows_dir[] DECLSPEC_HIDDEN;
extern const WCHAR system_dir[] DECLSPEC_HIDDEN;
extern const WCHAR syswow64_dir[] DECLSPEC_HIDDEN;

extern const struct unix_funcs *unix_funcs DECLSPEC_HIDDEN;

/* file I/O */
extern NTSTATUS server_get_unix_name( HANDLE handle, ANSI_STRING *unix_name ) DECLSPEC_HIDDEN;
extern void init_directories(void) DECLSPEC_HIDDEN;

extern struct _KUSER_SHARED_DATA *user_shared_data DECLSPEC_HIDDEN;

/* locale */
extern LCID user_lcid, system_lcid;
extern DWORD ntdll_umbstowcs( const char* src, DWORD srclen, WCHAR* dst, DWORD dstlen ) DECLSPEC_HIDDEN;
extern int ntdll_wcstoumbs( const WCHAR* src, DWORD srclen, char* dst, DWORD dstlen, BOOL strict ) DECLSPEC_HIDDEN;

extern int CDECL NTDLL__vsnprintf( char *str, SIZE_T len, const char *format, __ms_va_list args ) DECLSPEC_HIDDEN;
extern int CDECL NTDLL__vsnwprintf( WCHAR *str, SIZE_T len, const WCHAR *format, __ms_va_list args ) DECLSPEC_HIDDEN;

/* load order */

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

extern enum loadorder get_load_order( const WCHAR *app_name, const UNICODE_STRING *nt_name ) DECLSPEC_HIDDEN;

/* thread private data, stored in NtCurrentTeb()->GdiTebBatch */
struct ntdll_thread_data
{
    struct debug_info *debug_info;    /* info for debugstr functions */
    void              *start_stack;   /* stack for thread startup */
    int                request_fd;    /* fd for sending server requests */
    int                reply_fd;      /* fd for receiving server replies */
    int                wait_fd[2];    /* fd for sleeping server requests */
    BOOL               wow64_redir;   /* Wow64 filesystem redirection flag */
    pthread_t          pthread_id;    /* pthread thread id */
};

C_ASSERT( sizeof(struct ntdll_thread_data) <= sizeof(((TEB *)0)->GdiTebBatch) );

static inline struct ntdll_thread_data *ntdll_get_thread_data(void)
{
    return (struct ntdll_thread_data *)&NtCurrentTeb()->GdiTebBatch;
}

extern SYSTEM_CPU_INFORMATION cpu_info DECLSPEC_HIDDEN;

#define HASH_STRING_ALGORITHM_DEFAULT  0
#define HASH_STRING_ALGORITHM_X65599   1
#define HASH_STRING_ALGORITHM_INVALID  0xffffffff

NTSTATUS WINAPI RtlHashUnicodeString(PCUNICODE_STRING,BOOLEAN,ULONG,ULONG*);
void     WINAPI LdrInitializeThunk(CONTEXT*,void**,ULONG_PTR,ULONG_PTR);

#ifndef __GCC_HAVE_SYNC_COMPARE_AND_SWAP_8
#define InterlockedCompareExchange64(dest,xchg,cmp) RtlInterlockedCompareExchange64(dest,xchg,cmp)
#endif

/* string functions */
int    __cdecl NTDLL_tolower( int c );
int    __cdecl _stricmp( LPCSTR str1, LPCSTR str2 );
int    __cdecl NTDLL__wcsicmp( LPCWSTR str1, LPCWSTR str2 );
int    __cdecl NTDLL__wcsnicmp( LPCWSTR str1, LPCWSTR str2, size_t n );
int    __cdecl NTDLL_wcscmp( LPCWSTR str1, LPCWSTR str2 );
int    __cdecl NTDLL_wcsncmp( LPCWSTR str1, LPCWSTR str2, size_t n );
WCHAR  __cdecl NTDLL_towlower( WCHAR ch );
WCHAR  __cdecl NTDLL_towupper( WCHAR ch );
LPWSTR __cdecl NTDLL__wcslwr( LPWSTR str );
LPWSTR __cdecl NTDLL__wcsupr( LPWSTR str );
LPWSTR __cdecl NTDLL_wcscpy( LPWSTR dst, LPCWSTR src );
LPWSTR __cdecl NTDLL_wcscat( LPWSTR dst, LPCWSTR src );
LPWSTR __cdecl NTDLL_wcschr( LPCWSTR str, WCHAR ch );
size_t __cdecl NTDLL_wcslen( LPCWSTR str );
size_t __cdecl NTDLL_wcscspn( LPCWSTR str, LPCWSTR reject );
LPWSTR __cdecl NTDLL_wcsncat( LPWSTR s1, LPCWSTR s2, size_t n );
LPWSTR __cdecl NTDLL_wcsncpy( LPWSTR s1, LPCWSTR s2, size_t n );
LPWSTR __cdecl NTDLL_wcspbrk( LPCWSTR str, LPCWSTR accept );
LPWSTR __cdecl NTDLL_wcsrchr( LPCWSTR str, WCHAR ch );
size_t __cdecl NTDLL_wcsspn( LPCWSTR str, LPCWSTR accept );
LPWSTR __cdecl NTDLL_wcsstr( LPCWSTR str, LPCWSTR sub );
LPWSTR __cdecl NTDLL_wcstok( LPWSTR str, LPCWSTR delim );
LONG   __cdecl NTDLL_wcstol( LPCWSTR s, LPWSTR *end, INT base );
ULONG  __cdecl NTDLL_wcstoul( LPCWSTR s, LPWSTR *end, INT base );
int    WINAPIV NTDLL_swprintf( WCHAR *str, const WCHAR *format, ... );
int    WINAPIV _snwprintf_s( WCHAR *str, SIZE_T size, SIZE_T len, const WCHAR *format, ... );

#define wcsicmp(s1,s2) NTDLL__wcsicmp(s1,s2)
#define wcsnicmp(s1,s2,n) NTDLL__wcsnicmp(s1,s2,n)
#define towupper(c) NTDLL_towupper(c)
#define wcslwr(s) NTDLL__wcslwr(s)
#define wcsupr(s) NTDLL__wcsupr(s)
#define wcscpy(d,s) NTDLL_wcscpy(d,s)
#define wcscat(d,s) NTDLL_wcscat(d,s)
#define wcschr(s,c) NTDLL_wcschr(s,c)
#define wcspbrk(s,a) NTDLL_wcspbrk(s,a)
#define wcsrchr(s,c) NTDLL_wcsrchr(s,c)
#define wcstoul(s,e,b) NTDLL_wcstoul(s,e,b)
#define wcslen(s) NTDLL_wcslen(s)
#define wcscspn(s,r) NTDLL_wcscspn(s,r)
#define wcsspn(s,a) NTDLL_wcsspn(s,a)
#define wcscmp(s1,s2) NTDLL_wcscmp(s1,s2)
#define wcsncmp(s1,s2,n) NTDLL_wcsncmp(s1,s2,n)

/* convert from straight ASCII to Unicode without depending on the current codepage */
static inline void ascii_to_unicode( WCHAR *dst, const char *src, size_t len )
{
    while (len--) *dst++ = (unsigned char)*src++;
}

#endif
