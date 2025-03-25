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
#include "rtlsupportapi.h"
#include "unixlib.h"
#include "wine/asm.h"

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

static const UINT_PTR page_size = 0x1000;

/* exceptions */
extern NTSTATUS call_seh_handlers( EXCEPTION_RECORD *rec, CONTEXT *context );
extern NTSTATUS WINAPI dispatch_exception( EXCEPTION_RECORD *rec, CONTEXT *context );
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
extern void WINAPI KiUserExceptionDispatcher(EXCEPTION_RECORD*,CONTEXT*);
extern void WINAPI KiUserApcDispatcher(CONTEXT*,ULONG_PTR,ULONG_PTR,ULONG_PTR,PNTAPCFUNC);
extern void WINAPI KiUserCallbackDispatcher(ULONG,void*,ULONG);
extern void WINAPI KiUserCallbackDispatcherReturn(void);
extern void (WINAPI *pWow64PrepareForException)( EXCEPTION_RECORD *rec, CONTEXT *context );

/* debug helpers */
extern LPCSTR debugstr_us( const UNICODE_STRING *str );
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

#ifdef _WIN64
static inline TEB64 *NtCurrentTeb64(void) { return NULL; }
#else
static inline TEB64 *NtCurrentTeb64(void) { return (TEB64 *)NtCurrentTeb()->GdiBatchCount; }
#endif

static inline void *get_rva( HMODULE module, DWORD va )
{
    return (void *)((char *)module + va);
}

/* convert from straight ASCII to Unicode without depending on the current codepage */
static inline void ascii_to_unicode( WCHAR *dst, const char *src, size_t len )
{
    while (len--) *dst++ = (unsigned char)*src++;
}

/* FLS data */
extern TEB_FLS_DATA *fls_alloc_data(void);
extern void heap_thread_detach(void);

/* register context */

#ifdef __i386__
# define TRACE_CONTEXT(c) do { \
    TRACE( "eip=%08lx esp=%08lx ebp=%08lx eflags=%08lx\n", (c)->Eip, (c)->Esp, (c)->Ebp, (c)->EFlags );\
    TRACE( "eax=%08lx ebx=%08lx ecx=%08lx edx=%08lx\n", (c)->Eax, (c)->Ebx, (c)->Ecx, (c)->Edx ); \
    TRACE( "esi=%08lx edi=%08lx cs=%04x ds=%04x es=%04x fs=%04x gs=%04x ss=%04x\n", \
           (c)->Esi, (c)->Edi, LOWORD((c)->SegCs), LOWORD((c)->SegDs), LOWORD((c)->SegEs), \
           LOWORD((c)->SegFs), LOWORD((c)->SegGs), LOWORD((c)->SegSs) ); \
    } while(0)
#elif defined(__x86_64__)
# define TRACE_CONTEXT(c) do { \
    TRACE( "rip=%016I64x rsp=%016I64x rbp=%016I64x eflags=%08lx\n", (c)->Rip, (c)->Rsp, (c)->Rbp, (c)->EFlags ); \
    TRACE( "rax=%016I64x rbx=%016I64x rcx=%016I64x rdx=%016I64x\n", (c)->Rax, (c)->Rbx, (c)->Rcx, (c)->Rdx ); \
    TRACE( "rsi=%016I64x rdi=%016I64x  r8=%016I64x  r9=%016I64x\n", (c)->Rsi, (c)->Rdi, (c)->R8, (c)->R9 ); \
    TRACE( "r10=%016I64x r11=%016I64x r12=%016I64x r13=%016I64x\n", (c)->R10, (c)->R11, (c)->R12, (c)->R13 ); \
    TRACE( "r14=%016I64x r15=%016I64x mxcsr=%08lx\n", (c)->R14, (c)->R15, (c)->MxCsr ); \
    } while(0)
#elif defined(__arm__)
# define TRACE_CONTEXT(c) do { \
    TRACE( "pc=%08lx sp=%08lx lr=%08lx ip=%08lx cpsr=%08lx\n", (c)->Pc, (c)->Sp, (c)->Lr, (c)->R12, (c)->Cpsr ); \
    TRACE( "r0=%08lx r1=%08lx r2=%08lx r3=%08lx r4=%08lx r5=%08lx\n", (c)->R0, (c)->R1, (c)->R2, (c)->R3, (c)->R4, (c)->R5 ); \
    TRACE( "r6=%08lx r7=%08lx r8=%08lx r9=%08lx r10=%08lx r11=%08lx\n", (c)->R6, (c)->R7, (c)->R8, (c)->R9, (c)->R10, (c)->R11 ); \
    } while(0)
#elif defined(__aarch64__)
# define TRACE_CONTEXT(c) do { \
    TRACE( " pc=%016I64x  sp=%016I64x  lr=%016I64x  fp=%016I64x\n", (c)->Pc, (c)->Sp, (c)->Lr, (c)->Fp ); \
    TRACE( " x0=%016I64x  x1=%016I64x  x2=%016I64x  x3=%016I64x\n", (c)->X0, (c)->X1, (c)->X2, (c)->X3 ); \
    TRACE( " x4=%016I64x  x5=%016I64x  x6=%016I64x  x7=%016I64x\n", (c)->X4, (c)->X5, (c)->X6, (c)->X7 ); \
    TRACE( " x8=%016I64x  x9=%016I64x x10=%016I64x x11=%016I64x\n", (c)->X8, (c)->X9, (c)->X10, (c)->X11 ); \
    TRACE( "x12=%016I64x x13=%016I64x x14=%016I64x x15=%016I64x\n", (c)->X12, (c)->X13, (c)->X14, (c)->X15 ); \
    TRACE( "x16=%016I64x x17=%016I64x x18=%016I64x x19=%016I64x\n", (c)->X16, (c)->X17, (c)->X18, (c)->X19 ); \
    TRACE( "x20=%016I64x x21=%016I64x x22=%016I64x x23=%016I64x\n", (c)->X20, (c)->X21, (c)->X22, (c)->X23 ); \
    TRACE( "x24=%016I64x x25=%016I64x x26=%016I64x x27=%016I64x\n", (c)->X24, (c)->X25, (c)->X26, (c)->X27 ); \
    TRACE( "x28=%016I64x cpsr=%08lx fpcr=%08lx fpsr=%08lx\n", (c)->X28, (c)->Cpsr, (c)->Fpcr, (c)->Fpsr ); \
    } while(0)
#endif

#ifdef __arm64ec__

extern NTSTATUS arm64ec_process_init( HMODULE module );
extern NTSTATUS arm64ec_thread_init(void);
extern IMAGE_ARM64EC_METADATA *arm64ec_get_module_metadata( HMODULE module );
extern void arm64ec_update_hybrid_metadata( void *module, IMAGE_NT_HEADERS *nt,
                                            const IMAGE_ARM64EC_METADATA *metadata );
extern void invoke_arm64ec_syscall(void);

extern void *__os_arm64x_check_call;
extern void *__os_arm64x_check_icall;
extern void *__os_arm64x_check_icall_cfg;
extern void *__os_arm64x_dispatch_call_no_redirect;
extern void *__os_arm64x_dispatch_fptr;
extern void *__os_arm64x_dispatch_ret;
extern void *__os_arm64x_get_x64_information;
extern void *__os_arm64x_set_x64_information;
extern void *__os_arm64x_helper3;
extern void *__os_arm64x_helper4;
extern void *__os_arm64x_helper5;
extern void *__os_arm64x_helper6;
extern void *__os_arm64x_helper7;
extern void *__os_arm64x_helper8;

#endif

#endif
