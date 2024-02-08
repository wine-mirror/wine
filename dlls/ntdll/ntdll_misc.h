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
#include <stdlib.h>
#include <sys/types.h>

#include "windef.h"
#include "winbase.h"
#include "winnt.h"
#include "winternl.h"
#include "unixlib.h"
#include "wine/asm.h"

#define DECLARE_CRITICAL_SECTION(cs) \
    static RTL_CRITICAL_SECTION cs; \
    static RTL_CRITICAL_SECTION_DEBUG cs##_debug = \
    { 0, 0, &cs, { &cs##_debug.ProcessLocksList, &cs##_debug.ProcessLocksList }, \
      0, 0, { (DWORD_PTR)(__FILE__ ": " # cs) }}; \
    static RTL_CRITICAL_SECTION cs = { &cs##_debug, -1, 0, 0, 0, 0 };

#define MAX_NT_PATH_LENGTH 277

#define NTDLL_TLS_ERRNO 16  /* TLS slot for _errno() */

#ifdef __i386__
static const USHORT current_machine = IMAGE_FILE_MACHINE_I386;
#elif defined(__x86_64__)
static const USHORT current_machine = IMAGE_FILE_MACHINE_AMD64;
#elif defined(__arm__)
static const USHORT current_machine = IMAGE_FILE_MACHINE_ARMNT;
#elif defined(__aarch64__)
static const USHORT current_machine = IMAGE_FILE_MACHINE_ARM64;
#else
static const USHORT current_machine = IMAGE_FILE_MACHINE_UNKNOWN;
#endif

#if defined(__i386__) || defined(__x86_64__) || defined(__arm__) || defined(__aarch64__)
static const UINT_PTR page_size = 0x1000;
#else
extern UINT_PTR page_size;
#endif

/* exceptions */
extern LONG call_vectored_handlers( EXCEPTION_RECORD *rec, CONTEXT *context );
extern NTSTATUS WINAPI dispatch_user_callback( void *args, ULONG len, ULONG id );
extern EXCEPTION_DISPOSITION WINAPI user_callback_handler( EXCEPTION_RECORD *record, void *frame,
                                                           CONTEXT *context, void *dispatch );
extern EXCEPTION_DISPOSITION WINAPI nested_exception_handler( EXCEPTION_RECORD *rec, void *frame,
                                                              CONTEXT *context, void *dispatch );
extern void DECLSPEC_NORETURN raise_status( NTSTATUS status, EXCEPTION_RECORD *rec );
extern LONG WINAPI call_unhandled_exception_filter( PEXCEPTION_POINTERS eptr );
extern void WINAPI process_breakpoint(void);

static inline BOOL is_valid_frame( ULONG_PTR frame )
{
    if (frame & (sizeof(void*) - 1)) return FALSE;
    return ((void *)frame >= NtCurrentTeb()->Tib.StackLimit &&
            (void *)frame <= NtCurrentTeb()->Tib.StackBase);
}

extern void WINAPI LdrInitializeThunk(CONTEXT*,ULONG_PTR,ULONG_PTR,ULONG_PTR);
extern NTSTATUS WINAPI KiUserExceptionDispatcher(EXCEPTION_RECORD*,CONTEXT*);
extern void WINAPI KiUserApcDispatcher(CONTEXT*,ULONG_PTR,ULONG_PTR,ULONG_PTR,PNTAPCFUNC);
extern void WINAPI KiUserCallbackDispatcher(ULONG,void*,ULONG);
extern void WINAPI KiUserCallbackDispatcherReturn(void);
extern void (WINAPI *pWow64PrepareForException)( EXCEPTION_RECORD *rec, CONTEXT *context );

#if defined(__x86_64__) || defined(__arm__) || defined(__aarch64__)
extern RUNTIME_FUNCTION *lookup_function_info( ULONG_PTR pc, ULONG_PTR *base, LDR_DATA_TABLE_ENTRY **module );
#endif

/* debug helpers */
extern LPCSTR debugstr_us( const UNICODE_STRING *str );
extern const char *debugstr_exception_code( DWORD code );
extern void set_native_thread_name( DWORD tid, const char *name );

/* init routines */
extern void loader_init( CONTEXT *context, void **entry );
extern void version_init(void);
extern void debug_init(void);
extern void actctx_init(void);
extern void locale_init(void);
extern void init_user_process_params(void);
extern void get_resource_lcids( LANGID *user, LANGID *user_neutral, LANGID *system );

/* module handling */
extern LIST_ENTRY tls_links;
extern FARPROC RELAY_GetProcAddress( HMODULE module, const IMAGE_EXPORT_DIRECTORY *exports,
                                     DWORD exp_size, FARPROC proc, DWORD ordinal, const WCHAR *user );
extern FARPROC SNOOP_GetProcAddress( HMODULE hmod, const IMAGE_EXPORT_DIRECTORY *exports, DWORD exp_size,
                                     FARPROC origfun, DWORD ordinal, const WCHAR *user );
extern void RELAY_SetupDLL( HMODULE hmod );
extern void SNOOP_SetupDLL( HMODULE hmod );
extern const WCHAR windows_dir[];
extern const WCHAR system_dir[];

extern void (FASTCALL *pBaseThreadInitThunk)(DWORD,LPTHREAD_START_ROUTINE,void *);

extern struct _KUSER_SHARED_DATA *user_shared_data;

extern int CDECL NTDLL__vsnprintf( char *str, SIZE_T len, const char *format, va_list args );
extern int CDECL NTDLL__vsnwprintf( WCHAR *str, SIZE_T len, const WCHAR *format, va_list args );

struct dllredirect_data
{
    ULONG size;
    ULONG flags;
    ULONG total_len;
    ULONG paths_count;
    ULONG paths_offset;
    struct { ULONG len; ULONG offset; } paths[1];
};

#define DLL_REDIRECT_PATH_INCLUDES_BASE_NAME                      1
#define DLL_REDIRECT_PATH_OMITS_ASSEMBLY_ROOT                     2
#define DLL_REDIRECT_PATH_EXPAND                                  4
#define DLL_REDIRECT_PATH_SYSTEM_DEFAULT_REDIRECTED_SYSTEM32_DLL  8

#ifdef _WIN64
static inline TEB64 *NtCurrentTeb64(void) { return NULL; }
#else
static inline TEB64 *NtCurrentTeb64(void) { return (TEB64 *)NtCurrentTeb()->GdiBatchCount; }
#endif

#define HASH_STRING_ALGORITHM_DEFAULT  0
#define HASH_STRING_ALGORITHM_X65599   1
#define HASH_STRING_ALGORITHM_INVALID  0xffffffff

NTSTATUS WINAPI RtlHashUnicodeString(PCUNICODE_STRING,BOOLEAN,ULONG,ULONG*);

/* convert from straight ASCII to Unicode without depending on the current codepage */
static inline void ascii_to_unicode( WCHAR *dst, const char *src, size_t len )
{
    while (len--) *dst++ = (unsigned char)*src++;
}

/* FLS data */
extern TEB_FLS_DATA *fls_alloc_data(void);
extern void heap_thread_detach(void);

#ifdef __arm64ec__

extern void *__os_arm64x_check_call;
extern void *__os_arm64x_check_icall;
extern void *__os_arm64x_check_icall_cfg;
extern void *__os_arm64x_dispatch_call_no_redirect;
extern void *__os_arm64x_dispatch_fptr;
extern void *__os_arm64x_dispatch_ret;
extern void *__os_arm64x_get_x64_information;
extern void *__os_arm64x_set_x64_information;

#endif

#endif
