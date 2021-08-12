/*
 * WoW64 syscall wrapping
 *
 * Copyright 2021 Alexandre Julliard
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

#include <stdarg.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winnt.h"
#include "winternl.h"
#include "wine/exception.h"
#include "wine/unixlib.h"
#include "wow64_private.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(wow);

USHORT native_machine = 0;
USHORT current_machine = 0;
ULONG_PTR args_alignment = 0;

typedef NTSTATUS (WINAPI *syscall_thunk)( UINT *args );

static const syscall_thunk syscall_thunks[] =
{
#define SYSCALL_ENTRY(func) wow64_ ## func,
    ALL_SYSCALLS
#undef SYSCALL_ENTRY
};

static const char *syscall_names[] =
{
#define SYSCALL_ENTRY(func) #func,
    ALL_SYSCALLS
#undef SYSCALL_ENTRY
};

static unsigned short syscall_map[1024];

/* header for Wow64AllocTemp blocks; probably not the right layout */
struct mem_header
{
    struct mem_header *next;
    void              *__pad;
    BYTE               data[1];
};

static void **pWow64Transition;
static void **p__wine_syscall_dispatcher;
static SYSTEM_DLL_INIT_BLOCK *pLdrSystemDllInitBlock;

/* cpu backend dll functions */
static void *   (WINAPI *pBTCpuGetBopCode)(void);
static void     (WINAPI *pBTCpuProcessInit)(void);
static void     (WINAPI *pBTCpuSimulate)(void);


void *dummy = RtlUnwind;

BOOL WINAPI DllMain( HINSTANCE inst, DWORD reason, void *reserved )
{
    if (reason == DLL_PROCESS_ATTACH) LdrDisableThreadCalloutsForDll( inst );
    return TRUE;
}

void __cdecl __wine_spec_unimplemented_stub( const char *module, const char *function )
{
    EXCEPTION_RECORD record;

    record.ExceptionCode    = EXCEPTION_WINE_STUB;
    record.ExceptionFlags   = EH_NONCONTINUABLE;
    record.ExceptionRecord  = NULL;
    record.ExceptionAddress = __wine_spec_unimplemented_stub;
    record.NumberParameters = 2;
    record.ExceptionInformation[0] = (ULONG_PTR)module;
    record.ExceptionInformation[1] = (ULONG_PTR)function;
    for (;;) RtlRaiseException( &record );
}


/**********************************************************************
 *           wow64_NtAddAtom
 */
NTSTATUS WINAPI wow64_NtAddAtom( UINT *args )
{
    const WCHAR *name = get_ptr( &args );
    ULONG len = get_ulong( &args );
    RTL_ATOM *atom = get_ptr( &args );

    return NtAddAtom( name, len, atom );
}


/**********************************************************************
 *           wow64_NtAllocateLocallyUniqueId
 */
NTSTATUS WINAPI wow64_NtAllocateLocallyUniqueId( UINT *args )
{
    LUID *luid = get_ptr( &args );

    return NtAllocateLocallyUniqueId( luid );
}


/**********************************************************************
 *           wow64_NtAllocateUuids
 */
NTSTATUS WINAPI wow64_NtAllocateUuids( UINT *args )
{
    ULARGE_INTEGER *time = get_ptr( &args );
    ULONG *delta = get_ptr( &args );
    ULONG *sequence = get_ptr( &args );
    UCHAR *seed = get_ptr( &args );

    return NtAllocateUuids( time, delta, sequence, seed );
}


/**********************************************************************
 *           wow64_NtClose
 */
NTSTATUS WINAPI wow64_NtClose( UINT *args )
{
    HANDLE handle = get_handle( &args );

    return NtClose( handle );
}


/**********************************************************************
 *           wow64_NtDeleteAtom
 */
NTSTATUS WINAPI wow64_NtDeleteAtom( UINT *args )
{
    RTL_ATOM atom = get_ulong( &args );

    return NtDeleteAtom( atom );
}


/**********************************************************************
 *           wow64_NtFindAtom
 */
NTSTATUS WINAPI wow64_NtFindAtom( UINT *args )
{
    const WCHAR *name = get_ptr( &args );
    ULONG len = get_ulong( &args );
    RTL_ATOM *atom = get_ptr( &args );

    return NtFindAtom( name, len, atom );
}


/**********************************************************************
 *           wow64_NtGetCurrentProcessorNumber
 */
NTSTATUS WINAPI wow64_NtGetCurrentProcessorNumber( UINT *args )
{
    return NtGetCurrentProcessorNumber();
}


/**********************************************************************
 *           wow64_NtQueryDefaultLocale
 */
NTSTATUS WINAPI wow64_NtQueryDefaultLocale( UINT *args )
{
    BOOLEAN user = get_ulong( &args );
    LCID *lcid = get_ptr( &args );

    return NtQueryDefaultLocale( user, lcid );
}


/**********************************************************************
 *           wow64_NtQueryDefaultUILanguage
 */
NTSTATUS WINAPI wow64_NtQueryDefaultUILanguage( UINT *args )
{
    LANGID *lang = get_ptr( &args );

    return NtQueryDefaultUILanguage( lang );
}


/**********************************************************************
 *           wow64_NtQueryInformationAtom
 */
NTSTATUS WINAPI wow64_NtQueryInformationAtom( UINT *args )
{
    RTL_ATOM atom = get_ulong( &args );
    ATOM_INFORMATION_CLASS class = get_ulong( &args );
    void *info = get_ptr( &args );
    ULONG len = get_ulong( &args );
    ULONG *retlen = get_ptr( &args );

    if (class != AtomBasicInformation) FIXME( "class %u not supported\n", class );
    return NtQueryInformationAtom( atom, class, info, len, retlen );
}


/**********************************************************************
 *           wow64_NtQueryInstallUILanguage
 */
NTSTATUS WINAPI wow64_NtQueryInstallUILanguage( UINT *args )
{
    LANGID *lang = get_ptr( &args );

    return NtQueryInstallUILanguage( lang );
}


/**********************************************************************
 *           wow64_NtSetDefaultLocale
 */
NTSTATUS WINAPI wow64_NtSetDefaultLocale( UINT *args )
{
    BOOLEAN user = get_ulong( &args );
    LCID lcid = get_ulong( &args );

    return NtSetDefaultLocale( user, lcid );
}


/**********************************************************************
 *           wow64_NtSetDefaultUILanguage
 */
NTSTATUS WINAPI wow64_NtSetDefaultUILanguage( UINT *args )
{
    LANGID lang = get_ulong( &args );

    return NtSetDefaultUILanguage( lang );
}


/**********************************************************************
 *           wow64___wine_dbg_write
 */
NTSTATUS WINAPI wow64___wine_dbg_write( UINT *args )
{
    const char *str = get_ptr( &args );
    ULONG len = get_ulong( &args );

    return __wine_dbg_write( str, len );
}


/**********************************************************************
 *           wow64___wine_unix_call
 */
NTSTATUS WINAPI wow64___wine_unix_call( UINT *args )
{
    unixlib_handle_t handle = get_ulong64( &args );
    unsigned int code = get_ulong( &args );
    void *args_ptr = get_ptr( &args );

    return __wine_unix_call( handle, code, args_ptr );
}


/**********************************************************************
 *           wow64_wine_server_call
 */
NTSTATUS WINAPI wow64_wine_server_call( UINT *args )
{
    struct __server_request_info32 *req32 = get_ptr( &args );

    unsigned int i;
    NTSTATUS status;
    struct __server_request_info req;

    req.u.req = req32->u.req;
    req.data_count = req32->data_count;
    for (i = 0; i < req.data_count; i++)
    {
        req.data[i].ptr = ULongToPtr( req32->data[i].ptr );
        req.data[i].size = req32->data[i].size;
    }
    req.reply_data = ULongToPtr( req32->reply_data );
    status = wine_server_call( &req );
    req32->u.reply = req.u.reply;
    return status;
}


/**********************************************************************
 *           get_syscall_num
 */
static DWORD get_syscall_num( const BYTE *syscall )
{
    DWORD id = ~0u;

    if (!syscall) return id;
    switch (current_machine)
    {
    case IMAGE_FILE_MACHINE_I386:
        if (syscall[0] == 0xb8 && syscall[5] == 0xba && syscall[10] == 0xff && syscall[11] == 0xd2)
            id = *(DWORD *)(syscall + 1);
        break;

    case IMAGE_FILE_MACHINE_ARM:
        if (*(WORD *)syscall == 0xb40f)
        {
            DWORD inst = *(DWORD *)((WORD *)syscall + 1);
            id = ((inst << 1) & 0x0800) + ((inst << 12) & 0xf000) +
                ((inst >> 20) & 0x0700) + ((inst >> 16) & 0x00ff);
        }
        break;
    }
    return id;
}


/**********************************************************************
 *           init_syscall_table
 */
static void init_syscall_table( HMODULE ntdll )
{
    const IMAGE_EXPORT_DIRECTORY *exports;
    const ULONG *functions, *names;
    const USHORT *ordinals;
    ULONG id, exp_size, exp_pos, wrap_pos;

    args_alignment = (current_machine == IMAGE_FILE_MACHINE_I386) ? sizeof(ULONG) : sizeof(ULONG64);

    exports = RtlImageDirectoryEntryToData( ntdll, TRUE, IMAGE_DIRECTORY_ENTRY_EXPORT, &exp_size );
    ordinals = get_rva( ntdll, exports->AddressOfNameOrdinals );
    functions = get_rva( ntdll, exports->AddressOfFunctions );
    names = get_rva( ntdll, exports->AddressOfNames );

    for (exp_pos = wrap_pos = 0; exp_pos < exports->NumberOfNames; exp_pos++)
    {
        char *name = get_rva( ntdll, names[exp_pos] );
        int res = -1;

        if (strncmp( name, "Nt", 2 ) && strncmp( name, "wine", 4 ) && strncmp( name, "__wine", 6 ))
            continue;  /* not a syscall */

        if ((id = get_syscall_num( get_rva( ntdll, functions[ordinals[exp_pos]] ))) == ~0u)
            continue; /* not a syscall */

        if (wrap_pos < ARRAY_SIZE(syscall_names))
            res = strcmp( name, syscall_names[wrap_pos] );

        if (!res)  /* got a match */
        {
            if (id < ARRAY_SIZE(syscall_map)) syscall_map[id] = wrap_pos++;
            else ERR( "invalid syscall id %04x for %s\n", id, name );
        }
        else if (res > 0)
        {
            FIXME( "no ntdll export for syscall %s\n", syscall_names[wrap_pos] );
            wrap_pos++;
            exp_pos--;  /* try again */
        }
        else FIXME( "missing wrapper for syscall %04x %s\n", id, name );
    }

    for ( ; wrap_pos < ARRAY_SIZE(syscall_thunks); wrap_pos++)
        FIXME( "no ntdll export for syscall %s\n", syscall_names[wrap_pos] );
}


/**********************************************************************
 *           load_cpu_dll
 */
static HMODULE load_cpu_dll(void)
{
    NTSTATUS status;
    HMODULE module;
    UNICODE_STRING str;
    WCHAR path[MAX_PATH];
    const WCHAR *dir, *name;

    switch (current_machine)
    {
    case IMAGE_FILE_MACHINE_I386:
        name = (native_machine == IMAGE_FILE_MACHINE_ARM64 ? L"xtajit.dll" : L"wow64cpu.dll");
        break;
    case IMAGE_FILE_MACHINE_ARM:
        name = L"wowarmhw.dll";
        break;
    default:
        ERR( "unsupported machine %04x\n", current_machine );
        RtlExitUserProcess( 1 );
    }

    dir = get_machine_wow64_dir( IMAGE_FILE_MACHINE_TARGET_HOST );

    swprintf( path, MAX_PATH, L"%s\\%s", dir, name );
    RtlInitUnicodeString( &str, path );
    if ((status = LdrLoadDll( NULL, 0, &str, &module )))
    {
        ERR( "failed to load CPU dll %x\n", status );
        NtTerminateProcess( GetCurrentProcess(), status );
    }
    return module;
}


/**********************************************************************
 *           process_init
 */
static DWORD WINAPI process_init( RTL_RUN_ONCE *once, void *param, void **context )
{
    HMODULE module;
    UNICODE_STRING str;

    RtlWow64GetProcessMachines( GetCurrentProcess(), &current_machine, &native_machine );
    if (!current_machine) current_machine = native_machine;

#define GET_PTR(name) p ## name = RtlFindExportedRoutineByName( module, #name )

    RtlInitUnicodeString( &str, L"ntdll.dll" );
    LdrGetDllHandle( NULL, 0, &str, &module );
    GET_PTR( LdrSystemDllInitBlock );

    module = (HMODULE)(ULONG_PTR)pLdrSystemDllInitBlock->ntdll_handle;
    GET_PTR( Wow64Transition );
    GET_PTR( __wine_syscall_dispatcher );
    init_syscall_table( module );

    module = load_cpu_dll();
    GET_PTR( BTCpuGetBopCode );
    GET_PTR( BTCpuProcessInit );
    GET_PTR( BTCpuSimulate );

    pBTCpuProcessInit();
    *pWow64Transition = *p__wine_syscall_dispatcher = pBTCpuGetBopCode();

    init_file_redirects();
    return TRUE;

#undef GET_PTR
}


/**********************************************************************
 *           thread_init
 */
static void thread_init(void)
{
    /* update initial context to jump to 32-bit LdrInitializeThunk (cf. 32-bit call_init_thunk) */
    switch (current_machine)
    {
    case IMAGE_FILE_MACHINE_I386:
        {
            I386_CONTEXT *ctx_ptr, ctx = { CONTEXT_I386_ALL };
            ULONG *stack;

            NtQueryInformationThread( GetCurrentThread(), ThreadWow64Context, &ctx, sizeof(ctx), NULL );
            ctx_ptr = (I386_CONTEXT *)ULongToPtr( ctx.Esp ) - 1;
            *ctx_ptr = ctx;

            stack = (ULONG *)ctx_ptr;
            *(--stack) = 0;
            *(--stack) = 0;
            *(--stack) = 0;
            *(--stack) = PtrToUlong( ctx_ptr );
            *(--stack) = 0xdeadbabe;
            ctx.Esp = PtrToUlong( stack );
            ctx.Eip = pLdrSystemDllInitBlock->pLdrInitializeThunk;
            NtSetInformationThread( GetCurrentThread(), ThreadWow64Context, &ctx, sizeof(ctx) );
        }
        break;

    case IMAGE_FILE_MACHINE_ARMNT:
        {
            ARM_CONTEXT *ctx_ptr, ctx = { CONTEXT_ARM_ALL };

            NtQueryInformationThread( GetCurrentThread(), ThreadWow64Context, &ctx, sizeof(ctx), NULL );
            ctx_ptr = (ARM_CONTEXT *)ULongToPtr( ctx.Sp & ~15 ) - 1;
            *ctx_ptr = ctx;

            ctx.R0 = PtrToUlong( ctx_ptr );
            ctx.Sp = PtrToUlong( ctx_ptr );
            ctx.Pc = pLdrSystemDllInitBlock->pLdrInitializeThunk;
            NtSetInformationThread( GetCurrentThread(), ThreadWow64Context, &ctx, sizeof(ctx) );
        }
        break;

    default:
        ERR( "not supported machine %x\n", current_machine );
        NtTerminateProcess( GetCurrentProcess(), STATUS_INVALID_IMAGE_FORMAT );
    }
}


/**********************************************************************
 *           free_temp_data
 */
static void free_temp_data(void)
{
    struct mem_header *next, *mem;

    for (mem = NtCurrentTeb()->TlsSlots[WOW64_TLS_TEMPLIST]; mem; mem = next)
    {
        next = mem->next;
        RtlFreeHeap( GetProcessHeap(), 0, mem );
    }
    NtCurrentTeb()->TlsSlots[WOW64_TLS_TEMPLIST] = NULL;
}


/**********************************************************************
 *           Wow64SystemServiceEx  (NTDLL.@)
 */
NTSTATUS WINAPI Wow64SystemServiceEx( UINT num, UINT *args )
{
    NTSTATUS status;

    if (num >= ARRAY_SIZE( syscall_map ) || !syscall_map[num])
    {
        ERR( "unsupported syscall %04x\n", num );
        return STATUS_INVALID_SYSTEM_SERVICE;
    }
    __TRY
    {
        syscall_thunk thunk = syscall_thunks[syscall_map[num]];
        status = thunk( args );
    }
    __EXCEPT_ALL
    {
        status = GetExceptionCode();
    }
    __ENDTRY;
    free_temp_data();
    return status;
}


/**********************************************************************
 *           Wow64AllocateTemp
 *
 * FIXME: probably not 100% compatible.
 */
void * WINAPI Wow64AllocateTemp( SIZE_T size )
{
    struct mem_header *mem;

    if (!(mem = RtlAllocateHeap( GetProcessHeap(), 0, offsetof( struct mem_header, data[size] ))))
        return NULL;
    mem->next = NtCurrentTeb()->TlsSlots[WOW64_TLS_TEMPLIST];
    NtCurrentTeb()->TlsSlots[WOW64_TLS_TEMPLIST] = mem;
    return mem->data;
}


/**********************************************************************
 *           Wow64ApcRoutine  (NTDLL.@)
 */
void WINAPI Wow64ApcRoutine( ULONG_PTR arg1, ULONG_PTR arg2, ULONG_PTR arg3, CONTEXT *context )
{
    NTSTATUS retval;

#ifdef __x86_64__
    retval = context->Rax;
#elif defined(__aarch64__)
    retval = context->X0;
#endif

    /* cf. 32-bit call_user_apc_dispatcher */
    switch (current_machine)
    {
    case IMAGE_FILE_MACHINE_I386:
        {
            struct apc_stack_layout
            {
                ULONG         ret;
                ULONG         context_ptr;
                ULONG         arg1;
                ULONG         arg2;
                ULONG         arg3;
                ULONG         func;
                I386_CONTEXT  context;
            } *stack;
            I386_CONTEXT ctx = { CONTEXT_I386_FULL };

            NtQueryInformationThread( GetCurrentThread(), ThreadWow64Context, &ctx, sizeof(ctx), NULL );
            stack = (struct apc_stack_layout *)ULongToPtr( ctx.Esp & ~3 ) - 1;
            stack->context_ptr = PtrToUlong( &stack->context );
            stack->func = arg1 >> 32;
            stack->arg1 = arg1;
            stack->arg2 = arg2;
            stack->arg3 = arg3;
            stack->context = ctx;
            stack->context.Eax = retval;
            ctx.Esp = PtrToUlong( stack );
            ctx.Eip = pLdrSystemDllInitBlock->pKiUserApcDispatcher;
            NtSetInformationThread( GetCurrentThread(), ThreadWow64Context, &ctx, sizeof(ctx) );
        }
        break;

    case IMAGE_FILE_MACHINE_ARMNT:
        {
            struct apc_stack_layout
            {
                ULONG       func;
                ULONG       align[3];
                ARM_CONTEXT context;
            } *stack;
            ARM_CONTEXT ctx = { CONTEXT_ARM_FULL };

            NtQueryInformationThread( GetCurrentThread(), ThreadWow64Context, &ctx, sizeof(ctx), NULL );
            stack = (struct apc_stack_layout *)ULongToPtr( ctx.Sp & ~15 ) - 1;
            stack->func = arg1 >> 32;
            stack->context = ctx;
            stack->context.R0 = retval;
            ctx.Sp = PtrToUlong( stack );
            ctx.Pc = pLdrSystemDllInitBlock->pKiUserApcDispatcher;
            ctx.R0 = PtrToUlong( &stack->context );
            ctx.R1 = arg1;
            ctx.R2 = arg2;
            ctx.R3 = arg3;
            NtSetInformationThread( GetCurrentThread(), ThreadWow64Context, &ctx, sizeof(ctx) );
        }
        break;
    }
}


/**********************************************************************
 *           Wow64LdrpInitialize  (NTDLL.@)
 */
void WINAPI Wow64LdrpInitialize( CONTEXT *context )
{
    static RTL_RUN_ONCE init_done;

    RtlRunOnceExecuteOnce( &init_done, process_init, NULL, NULL );
    thread_init();
    pBTCpuSimulate();
}
