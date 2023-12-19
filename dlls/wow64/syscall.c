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
#include "wine/asm.h"
#include "wow64_private.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(wow);

USHORT native_machine = 0;
USHORT current_machine = 0;
ULONG_PTR args_alignment = 0;
ULONG_PTR highest_user_address = 0x7ffeffff;
ULONG_PTR default_zero_bits = 0x7fffffff;

typedef NTSTATUS (WINAPI *syscall_thunk)( UINT *args );

static const syscall_thunk syscall_thunks[] =
{
#define SYSCALL_ENTRY(id,name,args) wow64_ ## name,
    ALL_SYSCALLS32
#undef SYSCALL_ENTRY
};

static BYTE syscall_args[ARRAY_SIZE(syscall_thunks)] =
{
#define SYSCALL_ENTRY(id,name,args) args,
    ALL_SYSCALLS32
#undef SYSCALL_ENTRY
};

static SYSTEM_SERVICE_TABLE syscall_tables[4] =
{
    { (ULONG_PTR *)syscall_thunks, NULL, ARRAY_SIZE(syscall_thunks), syscall_args }
};

/* header for Wow64AllocTemp blocks; probably not the right layout */
struct mem_header
{
    struct mem_header *next;
    void              *__pad;
    BYTE               data[1];
};

/* stack frame for user callbacks */
struct user_callback_frame
{
    struct user_callback_frame *prev_frame;
    struct mem_header          *temp_list;
    void                      **ret_ptr;
    ULONG                      *ret_len;
    NTSTATUS                    status;
    __wine_jmp_buf              jmpbuf;
};

/* stack frame for user APCs */
struct user_apc_frame
{
    struct user_apc_frame *prev_frame;
    CONTEXT               *context;
    void                  *wow_context;
};

SYSTEM_DLL_INIT_BLOCK *pLdrSystemDllInitBlock = NULL;

static WOW64INFO *wow64info;

/* cpu backend dll functions */
/* the function prototypes most likely differ from Windows */
static void *   (WINAPI *pBTCpuGetBopCode)(void);
static NTSTATUS (WINAPI *pBTCpuGetContext)(HANDLE,HANDLE,void *,void *);
static BOOLEAN  (WINAPI *pBTCpuIsProcessorFeaturePresent)(UINT);
static void     (WINAPI *pBTCpuProcessInit)(void);
static NTSTATUS (WINAPI *pBTCpuSetContext)(HANDLE,HANDLE,void *,void *);
static void     (WINAPI *pBTCpuThreadInit)(void);
static void     (WINAPI *pBTCpuSimulate)(void);
static void *   (WINAPI *p__wine_get_unix_opcode)(void);
static void *   (WINAPI *pKiRaiseUserExceptionDispatcher)(void);
void (WINAPI *pBTCpuNotifyFlushInstructionCache2)( const void *, SIZE_T ) = NULL;
void (WINAPI *pBTCpuNotifyMapViewOfSection)( void * ) = NULL;
void (WINAPI *pBTCpuNotifyMemoryAlloc)( void *, SIZE_T, ULONG, ULONG ) = NULL;
void (WINAPI *pBTCpuNotifyMemoryDirty)( void *, SIZE_T ) = NULL;
void (WINAPI *pBTCpuNotifyMemoryFree)( void *, SIZE_T, ULONG ) = NULL;
void (WINAPI *pBTCpuNotifyMemoryProtect)( void *, SIZE_T, ULONG ) = NULL;
void (WINAPI *pBTCpuNotifyUnmapViewOfSection)( void * ) = NULL;
NTSTATUS (WINAPI *pBTCpuResetToConsistentState)( EXCEPTION_POINTERS * ) = NULL;
void (WINAPI *pBTCpuUpdateProcessorInformation)( SYSTEM_CPU_INFORMATION * ) = NULL;
void (WINAPI *pBTCpuThreadTerm)( HANDLE ) = NULL;

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


static EXCEPTION_RECORD *exception_record_32to64( const EXCEPTION_RECORD32 *rec32 )
{
    EXCEPTION_RECORD *rec;
    unsigned int i;

    rec = Wow64AllocateTemp( sizeof(*rec) );
    rec->ExceptionCode = rec32->ExceptionCode;
    rec->ExceptionFlags = rec32->ExceptionFlags;
    rec->ExceptionRecord = rec32->ExceptionRecord ? exception_record_32to64( ULongToPtr(rec32->ExceptionRecord) ) : NULL;
    rec->ExceptionAddress = ULongToPtr( rec32->ExceptionAddress );
    rec->NumberParameters = rec32->NumberParameters;
    for (i = 0; i < EXCEPTION_MAXIMUM_PARAMETERS; i++)
        rec->ExceptionInformation[i] = rec32->ExceptionInformation[i];
    return rec;
}


static void exception_record_64to32( EXCEPTION_RECORD32 *rec32, const EXCEPTION_RECORD *rec )
{
    unsigned int i;

    rec32->ExceptionCode    = rec->ExceptionCode;
    rec32->ExceptionFlags   = rec->ExceptionFlags;
    rec32->ExceptionRecord  = PtrToUlong( rec->ExceptionRecord );
    rec32->ExceptionAddress = PtrToUlong( rec->ExceptionAddress );
    rec32->NumberParameters = rec->NumberParameters;
    for (i = 0; i < rec->NumberParameters; i++)
        rec32->ExceptionInformation[i] = rec->ExceptionInformation[i];
}


static NTSTATUS get_context_return_value( void *wow_context )
{
    switch (current_machine)
    {
    case IMAGE_FILE_MACHINE_I386:
        return ((I386_CONTEXT *)wow_context)->Eax;
    case IMAGE_FILE_MACHINE_ARMNT:
        return ((ARM_CONTEXT *)wow_context)->R0;
    }
    return 0;
}


/**********************************************************************
 *           call_user_exception_dispatcher
 */
static void call_user_exception_dispatcher( EXCEPTION_RECORD32 *rec, void *ctx32_ptr, void *ctx64_ptr )
{
    switch (current_machine)
    {
    case IMAGE_FILE_MACHINE_I386:
        {
            /* stack layout when calling 32-bit KiUserExceptionDispatcher */
            struct exc_stack_layout32
            {
                ULONG              rec_ptr;       /* 000 */
                ULONG              context_ptr;   /* 004 */
                EXCEPTION_RECORD32 rec;           /* 008 */
                I386_CONTEXT       context;       /* 058 */
                CONTEXT_EX32       context_ex;    /* 324 */
                BYTE xstate[sizeof(XSTATE)+64];   /* 33c */
                DWORD             align;          /* 4bc */
            } *stack;
            I386_CONTEXT ctx = { CONTEXT_I386_ALL };
            CONTEXT_EX *context_ex, *src_ex = NULL;
            ULONG flags;

            C_ASSERT( offsetof(struct exc_stack_layout32, context) == 0x58 );
            C_ASSERT( offsetof(struct exc_stack_layout32, xstate) == 0x33c );
            C_ASSERT( sizeof(struct exc_stack_layout32) == 0x4c0 );

            pBTCpuGetContext( GetCurrentThread(), GetCurrentProcess(), NULL, &ctx );

            if (ctx32_ptr)
            {
                I386_CONTEXT *ctx32 = ctx32_ptr;

                if ((ctx32->ContextFlags & CONTEXT_I386_XSTATE) == CONTEXT_I386_XSTATE)
                    src_ex = (CONTEXT_EX *)(ctx32 + 1);
            }
            else if (native_machine == IMAGE_FILE_MACHINE_AMD64)
            {
                AMD64_CONTEXT *ctx64 = ctx64_ptr;

                if ((ctx64->ContextFlags & CONTEXT_AMD64_FLOATING_POINT) == CONTEXT_AMD64_FLOATING_POINT)
                    memcpy( ctx.ExtendedRegisters, &ctx64->FltSave, sizeof(ctx.ExtendedRegisters) );
                if ((ctx64->ContextFlags & CONTEXT_AMD64_XSTATE) == CONTEXT_AMD64_XSTATE)
                    src_ex = (CONTEXT_EX *)(ctx64 + 1);
            }

            flags = ctx.ContextFlags;
            if (src_ex) flags |= CONTEXT_I386_XSTATE;

            stack = (struct exc_stack_layout32 *)ULongToPtr( ctx.Esp & ~3 ) - 1;
            stack->rec_ptr     = PtrToUlong( &stack->rec );
            stack->context_ptr = PtrToUlong( &stack->context );
            stack->rec         = *rec;
            stack->context     = ctx;
            RtlInitializeExtendedContext( &stack->context, flags, &context_ex );
            /* adjust Eip for breakpoints in software emulation (hardware exceptions already adjust Rip) */
            if (rec->ExceptionCode == EXCEPTION_BREAKPOINT && (wow64info->CpuFlags & WOW64_CPUFLAGS_SOFTWARE))
                stack->context.Eip--;

            if (src_ex)
            {
                XSTATE *src_xs = (XSTATE *)((char *)src_ex + src_ex->XState.Offset);
                XSTATE *dst_xs = (XSTATE *)((char *)context_ex + context_ex->XState.Offset);

                dst_xs->Mask = src_xs->Mask & ~(ULONG64)3;
                dst_xs->CompactionMask = src_xs->CompactionMask;
                if ((dst_xs->Mask & 4) &&
                    src_ex->XState.Length >= sizeof(XSTATE) &&
                    context_ex->XState.Length >= sizeof(XSTATE))
                    memcpy( &dst_xs->YmmContext, &src_xs->YmmContext, sizeof(dst_xs->YmmContext) );
            }

            ctx.Esp = PtrToUlong( stack );
            ctx.Eip = pLdrSystemDllInitBlock->pKiUserExceptionDispatcher;
            ctx.EFlags &= ~(0x100|0x400|0x40000);
            pBTCpuSetContext( GetCurrentThread(), GetCurrentProcess(), NULL, &ctx );

            TRACE( "exception %08lx dispatcher %08lx stack %08lx eip %08lx\n",
                   rec->ExceptionCode, ctx.Eip, ctx.Esp, stack->context.Eip );
        }
        break;

    case IMAGE_FILE_MACHINE_ARMNT:
        {
            struct stack_layout
            {
                ARM_CONTEXT        context;
                EXCEPTION_RECORD32 rec;
            } *stack;
            ARM_CONTEXT ctx = { CONTEXT_ARM_ALL };

            pBTCpuGetContext( GetCurrentThread(), GetCurrentProcess(), NULL, &ctx );
            stack = (struct stack_layout *)(ULONG_PTR)(ctx.Sp & ~3) - 1;
            stack->rec = *rec;
            stack->context = ctx;

            ctx.R0 = PtrToUlong( &stack->rec );     /* first arg for KiUserExceptionDispatcher */
            ctx.R1 = PtrToUlong( &stack->context ); /* second arg for KiUserExceptionDispatcher */
            ctx.Sp = PtrToUlong( stack );
            ctx.Pc = pLdrSystemDllInitBlock->pKiUserExceptionDispatcher;
            if (ctx.Pc & 1) ctx.Cpsr |= 0x20;
            else ctx.Cpsr &= ~0x20;
            pBTCpuSetContext( GetCurrentThread(), GetCurrentProcess(), NULL, &ctx );

            TRACE( "exception %08lx dispatcher %08lx stack %08lx pc %08lx\n",
                   rec->ExceptionCode, ctx.Pc, ctx.Sp, stack->context.Sp );
        }
        break;
    }
}


/**********************************************************************
 *           call_raise_user_exception_dispatcher
 */
static void call_raise_user_exception_dispatcher( ULONG code )
{
    TEB32 *teb32 = (TEB32 *)((char *)NtCurrentTeb() + NtCurrentTeb()->WowTebOffset);

    teb32->ExceptionCode = code;

    switch (current_machine)
    {
    case IMAGE_FILE_MACHINE_I386:
        {
            I386_CONTEXT ctx = { CONTEXT_I386_ALL };

            pBTCpuGetContext( GetCurrentThread(), GetCurrentProcess(), NULL, &ctx );
            ctx.Esp -= sizeof(ULONG);
            *(ULONG *)ULongToPtr( ctx.Esp ) = ctx.Eip;
            ctx.Eip = (ULONG_PTR)pKiRaiseUserExceptionDispatcher;
            pBTCpuSetContext( GetCurrentThread(), GetCurrentProcess(), NULL, &ctx );
        }
        break;

    case IMAGE_FILE_MACHINE_ARMNT:
        {
            ARM_CONTEXT ctx = { CONTEXT_ARM_ALL };

            pBTCpuGetContext( GetCurrentThread(), GetCurrentProcess(), NULL, &ctx );
            ctx.Pc = (ULONG_PTR)pKiRaiseUserExceptionDispatcher;
            pBTCpuSetContext( GetCurrentThread(), GetCurrentProcess(), NULL, &ctx );
        }
        break;
    }
}


/* based on RtlRaiseException: call NtRaiseException with context setup to return to caller */
void WINAPI raise_exception( EXCEPTION_RECORD *rec, CONTEXT *context, BOOL first_chance );
#ifdef __x86_64__
__ASM_GLOBAL_FUNC( raise_exception,
                   "sub $0x28,%rsp\n\t"
                   __ASM_SEH(".seh_stackalloc 0x28\n\t")
                   __ASM_SEH(".seh_endprologue\n\t")
                   __ASM_CFI(".cfi_adjust_cfa_offset 0x28\n\t")
                   "movq %rcx,(%rsp)\n\t"
                   "movq %rdx,%rcx\n\t"
                   "call " __ASM_NAME("RtlCaptureContext") "\n\t"
                   "leaq 0x30(%rsp),%rax\n\t"   /* orig stack pointer */
                   "movq %rax,0x98(%rdx)\n\t"   /* context->Rsp */
                   "movq (%rsp),%rcx\n\t"       /* original first parameter */
                   "movq 0x28(%rsp),%rax\n\t"   /* return address */
                   "movq %rax,0xf8(%rdx)\n\t"   /* context->Rip */
                   "call " __ASM_NAME("NtRaiseException") )
#elif defined(__aarch64__)
__ASM_GLOBAL_FUNC( raise_exception,
                   "stp x29, x30, [sp, #-32]!\n\t"
                   __ASM_SEH(".seh_save_fplr_x 32\n\t")
                   __ASM_SEH(".seh_endprologue\n\t")
                   __ASM_CFI(".cfi_def_cfa x29, 32\n\t")
                   __ASM_CFI(".cfi_offset x30, -24\n\t")
                   __ASM_CFI(".cfi_offset x29, -32\n\t")
                   "mov x29, sp\n\t"
                   "stp x0, x1, [sp, #16]\n\t"
                   "mov x0, x1\n\t"
                   "bl " __ASM_NAME("RtlCaptureContext") "\n\t"
                   "ldp x0, x1, [sp, #16]\n\t"    /* orig parameters */
                   "ldp x4, x5, [sp]\n\t"         /* frame pointer, return address */
                   "stp x4, x5, [x1, #0xf0]\n\t"  /* context->Fp, Lr */
                   "add x4, sp, #32\n\t"          /* orig stack pointer */
                   "stp x4, x5, [x1, #0x100]\n\t" /* context->Sp, Pc */
                   "bl " __ASM_NAME("NtRaiseException") )
#endif


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


/***********************************************************************
 *           wow64_NtCallbackReturn
 */
NTSTATUS WINAPI wow64_NtCallbackReturn( UINT *args )
{
    void *ret_ptr = get_ptr( &args );
    ULONG ret_len = get_ulong( &args );
    NTSTATUS status = get_ulong( &args );

    struct user_callback_frame *frame = NtCurrentTeb()->TlsSlots[WOW64_TLS_USERCALLBACKDATA];

    if (!frame) return STATUS_NO_CALLBACK_ACTIVE;

    *frame->ret_ptr = ret_ptr;
    *frame->ret_len = ret_len;
    frame->status = status;
    __wine_longjmp( &frame->jmpbuf, 1 );
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
 *           wow64_NtContinue
 */
NTSTATUS WINAPI wow64_NtContinue( UINT *args )
{
    void *context = get_ptr( &args );
    BOOLEAN alertable = get_ulong( &args );

    NTSTATUS status = get_context_return_value( context );
    struct user_apc_frame *frame = NtCurrentTeb()->TlsSlots[WOW64_TLS_APCLIST];

    pBTCpuSetContext( GetCurrentThread(), GetCurrentProcess(), NULL, context );

    while (frame && frame->wow_context != context) frame = frame->prev_frame;
    NtCurrentTeb()->TlsSlots[WOW64_TLS_APCLIST] = frame ? frame->prev_frame : NULL;
    if (frame) NtContinue( frame->context, alertable );

    if (alertable) NtTestAlert();
    return status;
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
 *           wow64_NtGetContextThread
 */
NTSTATUS WINAPI wow64_NtGetContextThread( UINT *args )
{
    HANDLE handle = get_handle( &args );
    WOW64_CONTEXT *context = get_ptr( &args );

    return RtlWow64GetThreadContext( handle, context );
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
 *           wow64_NtRaiseException
 */
NTSTATUS WINAPI wow64_NtRaiseException( UINT *args )
{
    EXCEPTION_RECORD32 *rec32 = get_ptr( &args );
    void *context32 = get_ptr( &args );
    BOOL first_chance = get_ulong( &args );

    EXCEPTION_RECORD *rec = exception_record_32to64( rec32 );
    CONTEXT context;

    pBTCpuSetContext( GetCurrentThread(), GetCurrentProcess(), NULL, context32 );

    __TRY
    {
        raise_exception( rec, &context, first_chance );
    }
    __EXCEPT_ALL
    {
        call_user_exception_dispatcher( rec32, context32, &context );
    }
    __ENDTRY
    return STATUS_SUCCESS;
}


/**********************************************************************
 *           wow64_NtSetContextThread
 */
NTSTATUS WINAPI wow64_NtSetContextThread( UINT *args )
{
    HANDLE handle = get_handle( &args );
    WOW64_CONTEXT *context = get_ptr( &args );

    return RtlWow64SetThreadContext( handle, context );
}


/**********************************************************************
 *           wow64_NtSetDebugFilterState
 */
NTSTATUS WINAPI wow64_NtSetDebugFilterState( UINT *args )
{
    ULONG component_id = get_ulong( &args );
    ULONG level = get_ulong( &args );
    BOOLEAN state = get_ulong( &args );

    return NtSetDebugFilterState( component_id, level, state );
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
 *           wow64_NtWow64IsProcessorFeaturePresent
 */
NTSTATUS WINAPI wow64_NtWow64IsProcessorFeaturePresent( UINT *args )
{
    UINT feature = get_ulong( &args );

    return pBTCpuIsProcessorFeaturePresent && pBTCpuIsProcessorFeaturePresent( feature );
}


/**********************************************************************
 *           init_image_mapping
 */
void init_image_mapping( HMODULE module )
{
    ULONG *ptr = RtlFindExportedRoutineByName( module, "Wow64Transition" );

    if (ptr) *ptr = PtrToUlong( pBTCpuGetBopCode() );
}


/**********************************************************************
 *           load_64bit_module
 */
static HMODULE load_64bit_module( const WCHAR *name )
{
    NTSTATUS status;
    HMODULE module;
    UNICODE_STRING str;
    WCHAR path[MAX_PATH];
    const WCHAR *dir = get_machine_wow64_dir( IMAGE_FILE_MACHINE_TARGET_HOST );

    swprintf( path, MAX_PATH, L"%s\\%s", dir, name );
    RtlInitUnicodeString( &str, path );
    if ((status = LdrLoadDll( dir, 0, &str, &module )))
    {
        ERR( "failed to load dll %lx\n", status );
        NtTerminateProcess( GetCurrentProcess(), status );
    }
    return module;
}


/**********************************************************************
 *           get_cpu_dll_name
 */
static const WCHAR *get_cpu_dll_name(void)
{
    static ULONG buffer[32];
    KEY_VALUE_PARTIAL_INFORMATION *info = (KEY_VALUE_PARTIAL_INFORMATION *)buffer;
    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING nameW;
    const WCHAR *ret;
    HANDLE key;
    ULONG size;

    switch (current_machine)
    {
    case IMAGE_FILE_MACHINE_I386:
        RtlInitUnicodeString( &nameW, L"\\Registry\\Machine\\Software\\Microsoft\\Wow64\\x86" );
        ret = (native_machine == IMAGE_FILE_MACHINE_ARM64 ? L"xtajit.dll" : L"wow64cpu.dll");
        break;
    case IMAGE_FILE_MACHINE_ARMNT:
        RtlInitUnicodeString( &nameW, L"\\Registry\\Machine\\Software\\Microsoft\\Wow64\\arm" );
        ret = L"wowarmhw.dll";
        break;
    default:
        ERR( "unsupported machine %04x\n", current_machine );
        RtlExitUserProcess( 1 );
    }
    InitializeObjectAttributes( &attr, &nameW, OBJ_CASE_INSENSITIVE, 0, NULL );
    if (NtOpenKey( &key, KEY_READ | KEY_WOW64_64KEY, &attr )) return ret;
    RtlInitUnicodeString( &nameW, L"" );
    size = sizeof(buffer) - sizeof(WCHAR);
    if (!NtQueryValueKey( key, &nameW, KeyValuePartialInformation, buffer, size, &size ) && info->Type == REG_SZ)
    {
        ((WCHAR *)info->Data)[info->DataLength / sizeof(WCHAR)] = 0;
        ret = (WCHAR *)info->Data;
    }
    NtClose( key );
    return ret;
}


/**********************************************************************
 *           create_cross_process_work_list
 */
static NTSTATUS create_cross_process_work_list( WOW64INFO *wow64info )
{
    SIZE_T map_size = 0x4000;
    LARGE_INTEGER size;
    NTSTATUS status;
    HANDLE section;
    CROSS_PROCESS_WORK_LIST *list = NULL;
    CROSS_PROCESS_WORK_ENTRY *end;
    UINT i;

    size.QuadPart = map_size;
    status = NtCreateSection( &section, SECTION_ALL_ACCESS, NULL, &size, PAGE_READWRITE, SEC_COMMIT, 0 );
    if (status) return status;
    status = NtMapViewOfSection( section, GetCurrentProcess(), (void **)&list, default_zero_bits, 0, NULL,
                                 &map_size, ViewShare, MEM_TOP_DOWN, PAGE_READWRITE );
    if (status)
    {
        NtClose( section );
        return status;
    }

    end = (CROSS_PROCESS_WORK_ENTRY *)((char *)list + map_size);
    for (i = 0; list->entries + i + 1 <= end; i++)
        RtlWow64PushCrossProcessWorkOntoFreeList( &list->free_list, &list->entries[i] );

    wow64info->SectionHandle = (ULONG_PTR)section;
    wow64info->CrossProcessWorkList = (ULONG_PTR)list;
    return STATUS_SUCCESS;
}


/**********************************************************************
 *           process_init
 */
static DWORD WINAPI process_init( RTL_RUN_ONCE *once, void *param, void **context )
{
    PEB32 *peb32;
    HMODULE module;
    UNICODE_STRING str = RTL_CONSTANT_STRING( L"ntdll.dll" );
    SYSTEM_BASIC_INFORMATION info;
    ULONG *p__wine_syscall_dispatcher, *p__wine_unix_call_dispatcher;
    const SYSTEM_SERVICE_TABLE *psdwhwin32;

    RtlWow64GetProcessMachines( GetCurrentProcess(), &current_machine, &native_machine );
    if (!current_machine) current_machine = native_machine;
    args_alignment = (current_machine == IMAGE_FILE_MACHINE_I386) ? sizeof(ULONG) : sizeof(ULONG64);
    NtQuerySystemInformation( SystemEmulationBasicInformation, &info, sizeof(info), NULL );
    highest_user_address = (ULONG_PTR)info.HighestUserAddress;
    default_zero_bits = (ULONG_PTR)info.HighestUserAddress | 0x7fffffff;
    NtQueryInformationProcess( GetCurrentProcess(), ProcessWow64Information, &peb32, sizeof(peb32), NULL );
    wow64info = (WOW64INFO *)(peb32 + 1);
    wow64info->NativeSystemPageSize = 0x1000;
    wow64info->NativeMachineType    = native_machine;
    wow64info->EmulatedMachineType  = current_machine;
    NtCurrentTeb()->TlsSlots[WOW64_TLS_WOW64INFO] = wow64info;

#define GET_PTR(name) p ## name = RtlFindExportedRoutineByName( module, #name )

    LdrGetDllHandle( NULL, 0, &str, &module );
    GET_PTR( LdrSystemDllInitBlock );

    module = load_64bit_module( get_cpu_dll_name() );
    GET_PTR( BTCpuGetBopCode );
    GET_PTR( BTCpuGetContext );
    GET_PTR( BTCpuIsProcessorFeaturePresent );
    GET_PTR( BTCpuProcessInit );
    GET_PTR( BTCpuThreadInit );
    GET_PTR( BTCpuResetToConsistentState );
    GET_PTR( BTCpuSetContext );
    GET_PTR( BTCpuSimulate );
    GET_PTR( BTCpuNotifyFlushInstructionCache2 );
    GET_PTR( BTCpuNotifyMapViewOfSection );
    GET_PTR( BTCpuNotifyMemoryAlloc );
    GET_PTR( BTCpuNotifyMemoryDirty );
    GET_PTR( BTCpuNotifyMemoryFree );
    GET_PTR( BTCpuNotifyMemoryProtect );
    GET_PTR( BTCpuNotifyUnmapViewOfSection );
    GET_PTR( BTCpuUpdateProcessorInformation );
    GET_PTR( BTCpuThreadTerm );
    GET_PTR( __wine_get_unix_opcode );

    module = load_64bit_module( L"wow64win.dll" );
    GET_PTR( sdwhwin32 );
    syscall_tables[1] = *psdwhwin32;

    pBTCpuProcessInit();

    module = (HMODULE)(ULONG_PTR)pLdrSystemDllInitBlock->ntdll_handle;
    init_image_mapping( module );
    GET_PTR( KiRaiseUserExceptionDispatcher );
    GET_PTR( __wine_syscall_dispatcher );
    GET_PTR( __wine_unix_call_dispatcher );

    *p__wine_syscall_dispatcher = PtrToUlong( pBTCpuGetBopCode() );
    *p__wine_unix_call_dispatcher = PtrToUlong( p__wine_get_unix_opcode() );

    if (wow64info->CpuFlags & WOW64_CPUFLAGS_SOFTWARE) create_cross_process_work_list( wow64info );

    init_file_redirects();
    return TRUE;

#undef GET_PTR
}


/**********************************************************************
 *           thread_init
 */
static void thread_init(void)
{
    TEB32 *teb32 = (TEB32 *)((char *)NtCurrentTeb() + NtCurrentTeb()->WowTebOffset);
    void *cpu_area_ctx;

    teb32->WOW32Reserved = PtrToUlong( pBTCpuGetBopCode() );
    RtlWow64GetCurrentCpuArea( NULL, &cpu_area_ctx, NULL );
    NtCurrentTeb()->TlsSlots[WOW64_TLS_WOW64INFO] = wow64info;
    if (pBTCpuThreadInit) pBTCpuThreadInit();

    /* update initial context to jump to 32-bit LdrInitializeThunk (cf. 32-bit call_init_thunk) */
    switch (current_machine)
    {
    case IMAGE_FILE_MACHINE_I386:
        {
            I386_CONTEXT *ctx_ptr, *ctx = cpu_area_ctx;
            ULONG *stack;

            ctx_ptr = (I386_CONTEXT *)ULongToPtr( ctx->Esp ) - 1;
            *ctx_ptr = *ctx;

            stack = (ULONG *)ctx_ptr;
            *(--stack) = 0;
            *(--stack) = 0;
            *(--stack) = 0;
            *(--stack) = PtrToUlong( ctx_ptr );
            *(--stack) = 0xdeadbabe;
            ctx->Esp = PtrToUlong( stack );
            ctx->Eip = pLdrSystemDllInitBlock->pLdrInitializeThunk;
            pBTCpuSetContext( GetCurrentThread(), GetCurrentProcess(), NULL, ctx );
        }
        break;

    case IMAGE_FILE_MACHINE_ARMNT:
        {
            ARM_CONTEXT *ctx_ptr, *ctx = cpu_area_ctx;

            ctx_ptr = (ARM_CONTEXT *)ULongToPtr( ctx->Sp & ~15 ) - 1;
            *ctx_ptr = *ctx;

            ctx->R0 = PtrToUlong( ctx_ptr );
            ctx->Sp = PtrToUlong( ctx_ptr );
            ctx->Pc = pLdrSystemDllInitBlock->pLdrInitializeThunk;
            pBTCpuSetContext( GetCurrentThread(), GetCurrentProcess(), NULL, ctx );
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
 *           syscall_filter
 */
static LONG CALLBACK syscall_filter( EXCEPTION_POINTERS *ptrs )
{
    switch (ptrs->ExceptionRecord->ExceptionCode)
    {
    case STATUS_INVALID_HANDLE:
        call_raise_user_exception_dispatcher( ptrs->ExceptionRecord->ExceptionCode );
        break;
    }
    return EXCEPTION_EXECUTE_HANDLER;
}


/**********************************************************************
 *           Wow64SystemServiceEx  (wow64.@)
 */
NTSTATUS WINAPI Wow64SystemServiceEx( UINT num, UINT *args )
{
    NTSTATUS status;
    UINT id = num & 0xfff;
    const SYSTEM_SERVICE_TABLE *table = &syscall_tables[(num >> 12) & 3];

    if (id >= table->ServiceLimit || !table->ServiceTable[id])
    {
        ERR( "unsupported syscall %04x\n", num );
        return STATUS_INVALID_SYSTEM_SERVICE;
    }
    __TRY
    {
        syscall_thunk thunk = (syscall_thunk)table->ServiceTable[id];
        status = thunk( args );
    }
    __EXCEPT( syscall_filter )
    {
        status = GetExceptionCode();
    }
    __ENDTRY
    free_temp_data();
    return status;
}


/**********************************************************************
 *           simulate_filter
 */
static LONG CALLBACK simulate_filter( EXCEPTION_POINTERS *ptrs )
{
    Wow64PassExceptionToGuest( ptrs );
    return EXCEPTION_EXECUTE_HANDLER;
}


/**********************************************************************
 *           cpu_simulate
 */
static void cpu_simulate(void)
{
    for (;;)
    {
        __TRY
        {
            pBTCpuSimulate();
        }
        __EXCEPT( simulate_filter )
        {
            /* restart simulation loop */
        }
        __ENDTRY
    }
}


/**********************************************************************
 *           Wow64AllocateTemp  (wow64.@)
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
 *           Wow64ApcRoutine  (wow64.@)
 */
void WINAPI Wow64ApcRoutine( ULONG_PTR arg1, ULONG_PTR arg2, ULONG_PTR arg3, CONTEXT *context )
{
    struct user_apc_frame frame;

    frame.prev_frame = NtCurrentTeb()->TlsSlots[WOW64_TLS_APCLIST];
    frame.context    = context;
    NtCurrentTeb()->TlsSlots[WOW64_TLS_APCLIST] = &frame;

    /* cf. 32-bit call_user_apc_dispatcher */
    switch (current_machine)
    {
    case IMAGE_FILE_MACHINE_I386:
        {
            /* stack layout when calling 32-bit KiUserApcDispatcher */
            struct apc_stack_layout32
            {
                ULONG             func;          /* 000 */
                UINT              arg1;          /* 004 */
                UINT              arg2;          /* 008 */
                UINT              arg3;          /* 00c */
                UINT              alertable;     /* 010 */
                I386_CONTEXT      context;       /* 014 */
                CONTEXT_EX32      xctx;          /* 2e0 */
                UINT              unk2[4];       /* 2f8 */
            } *stack;
            I386_CONTEXT ctx = { CONTEXT_I386_FULL };

            C_ASSERT( offsetof(struct apc_stack_layout32, context) == 0x14 );
            C_ASSERT( sizeof(struct apc_stack_layout32) == 0x308 );

            pBTCpuGetContext( GetCurrentThread(), GetCurrentProcess(), NULL, &ctx );

            stack = (struct apc_stack_layout32 *)ULongToPtr( ctx.Esp & ~3 ) - 1;
            stack->func      = arg1 >> 32;
            stack->arg1      = arg1;
            stack->arg2      = arg2;
            stack->arg3      = arg3;
            stack->alertable = TRUE;
            stack->context   = ctx;
            stack->xctx.Legacy.Offset = -(LONG)sizeof(stack->context);
            stack->xctx.Legacy.Length = sizeof(stack->context);
            stack->xctx.All.Offset    = -(LONG)sizeof(stack->context);
            stack->xctx.All.Length    = sizeof(stack->context) + sizeof(stack->xctx);
            stack->xctx.XState.Offset = 25;
            stack->xctx.XState.Length = 0;

            ctx.Esp = PtrToUlong( stack );
            ctx.Eip = pLdrSystemDllInitBlock->pKiUserApcDispatcher;
            frame.wow_context = &stack->context;
            pBTCpuSetContext( GetCurrentThread(), GetCurrentProcess(), NULL, &ctx );
            cpu_simulate();
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

            pBTCpuGetContext( GetCurrentThread(), GetCurrentProcess(), NULL, &ctx );
            stack = (struct apc_stack_layout *)ULongToPtr( ctx.Sp & ~15 ) - 1;
            stack->func = arg1 >> 32;
            stack->context = ctx;
            ctx.Sp = PtrToUlong( stack );
            ctx.Pc = pLdrSystemDllInitBlock->pKiUserApcDispatcher;
            ctx.R0 = PtrToUlong( &stack->context );
            ctx.R1 = arg1;
            ctx.R2 = arg2;
            ctx.R3 = arg3;
            frame.wow_context = &stack->context;
            pBTCpuSetContext( GetCurrentThread(), GetCurrentProcess(), NULL, &ctx );
            cpu_simulate();
        }
        break;
    }
}


/**********************************************************************
 *           Wow64KiUserCallbackDispatcher  (wow64.@)
 */
NTSTATUS WINAPI Wow64KiUserCallbackDispatcher( ULONG id, void *args, ULONG len,
                                               void **ret_ptr, ULONG *ret_len )
{
    WOW64_CPURESERVED *cpu = NtCurrentTeb()->TlsSlots[WOW64_TLS_CPURESERVED];
    TEB32 *teb32 = (TEB32 *)((char *)NtCurrentTeb() + NtCurrentTeb()->WowTebOffset);
    ULONG teb_frame = teb32->Tib.ExceptionList;
    struct user_callback_frame frame;
    USHORT flags = cpu->Flags;

    frame.prev_frame = NtCurrentTeb()->TlsSlots[WOW64_TLS_USERCALLBACKDATA];
    frame.temp_list  = NtCurrentTeb()->TlsSlots[WOW64_TLS_TEMPLIST];
    frame.ret_ptr    = ret_ptr;
    frame.ret_len    = ret_len;

    NtCurrentTeb()->TlsSlots[WOW64_TLS_USERCALLBACKDATA] = &frame;
    NtCurrentTeb()->TlsSlots[WOW64_TLS_TEMPLIST] = NULL;

    /* cf. 32-bit KeUserModeCallback */
    switch (current_machine)
    {
    case IMAGE_FILE_MACHINE_I386:
        {
            /* stack layout when calling 32-bit KiUserCallbackDispatcher */
            struct callback_stack_layout32
            {
                ULONG             eip;           /* 000 */
                ULONG             id;            /* 004 */
                ULONG             args;          /* 008 */
                ULONG             len;           /* 00c */
                ULONG             unk[2];        /* 010 */
                ULONG             esp;           /* 018 */
                BYTE              args_data[0];  /* 01c */
            } *stack;
            I386_CONTEXT orig_ctx, ctx = { CONTEXT_I386_FULL };

            C_ASSERT( sizeof(struct callback_stack_layout32) == 0x1c );

            pBTCpuGetContext( GetCurrentThread(), GetCurrentProcess(), NULL, &ctx );
            orig_ctx = ctx;

            stack = ULongToPtr( (ctx.Esp - offsetof(struct callback_stack_layout32,args_data[len])) & ~15 );
            stack->eip  = ctx.Eip;
            stack->id   = id;
            stack->args = PtrToUlong( stack->args_data );
            stack->len  = len;
            stack->esp  = ctx.Esp;
            memcpy( stack->args_data, args, len );

            ctx.Esp = PtrToUlong( stack );
            ctx.Eip = pLdrSystemDllInitBlock->pKiUserCallbackDispatcher;
            pBTCpuSetContext( GetCurrentThread(), GetCurrentProcess(), NULL, &ctx );

            if (!__wine_setjmpex( &frame.jmpbuf, NULL ))
                cpu_simulate();
            else
                pBTCpuSetContext( GetCurrentThread(), GetCurrentProcess(), NULL, &orig_ctx );
        }
        break;

    case IMAGE_FILE_MACHINE_ARMNT:
        {
            ARM_CONTEXT orig_ctx, ctx = { CONTEXT_ARM_FULL };
            void *args_data;

            pBTCpuGetContext( GetCurrentThread(), GetCurrentProcess(), NULL, &ctx );
            orig_ctx = ctx;

            args_data = ULongToPtr( (ctx.Sp - len) & ~15 );
            memcpy( args_data, args, len );

            ctx.R0 = id;
            ctx.R1 = PtrToUlong( args_data );
            ctx.R2 = len;
            ctx.Sp = PtrToUlong( args_data );
            ctx.Pc = pLdrSystemDllInitBlock->pKiUserCallbackDispatcher;
            pBTCpuSetContext( GetCurrentThread(), GetCurrentProcess(), NULL, &ctx );

            if (!__wine_setjmpex( &frame.jmpbuf, NULL ))
                cpu_simulate();
            else
                pBTCpuSetContext( GetCurrentThread(), GetCurrentProcess(), NULL, &orig_ctx );
        }
        break;
    }

    teb32->Tib.ExceptionList = teb_frame;
    NtCurrentTeb()->TlsSlots[WOW64_TLS_USERCALLBACKDATA] = frame.prev_frame;
    NtCurrentTeb()->TlsSlots[WOW64_TLS_TEMPLIST] = frame.temp_list;
    cpu->Flags = flags;
    return frame.status;
}


/**********************************************************************
 *           Wow64LdrpInitialize  (wow64.@)
 */
void WINAPI Wow64LdrpInitialize( CONTEXT *context )
{
    static RTL_RUN_ONCE init_done;

    RtlRunOnceExecuteOnce( &init_done, process_init, NULL, NULL );
    thread_init();
    cpu_simulate();
}


/**********************************************************************
 *           Wow64PrepareForException  (wow64.@)
 */
#ifdef __x86_64__
__ASM_GLOBAL_FUNC( Wow64PrepareForException,
                   "sub $0x38,%rsp\n\t"
                   "mov %rcx,%r10\n\t"           /* rec */
                   "movw %cs,%ax\n\t"
                   "cmpw %ax,0x38(%rdx)\n\t"     /* context->SegCs */
                   "je 1f\n\t"                   /* already in 64-bit mode? */
                   /* copy arguments to 64-bit stack */
                   "mov %rsp,%rsi\n\t"
                   "mov 0x98(%rdx),%rcx\n\t"     /* context->Rsp */
                   "sub %rsi,%rcx\n\t"           /* stack size */
                   "sub %rcx,%r14\n\t"           /* reserve same size on 64-bit stack */
                   "and $~0x0f,%r14\n\t"
                   "mov %r14,%rdi\n\t"
                   "shr $3,%rcx\n\t"
                   "rep; movsq\n\t"
                   /* update arguments to point to the new stack */
                   "mov %r14,%rax\n\t"
                   "sub %rsp,%rax\n\t"
                   "add %rax,%r10\n\t"           /* rec */
                   "add %rax,%rdx\n\t"           /* context */
                   /* switch to 64-bit stack */
                   "mov %r14,%rsp\n"
                   /* build EXCEPTION_POINTERS structure and call BTCpuResetToConsistentState */
                   "1:\tlea 0x20(%rsp),%rcx\n\t" /* pointers */
                   "mov %r10,(%rcx)\n\t"         /* rec */
                   "mov %rdx,8(%rcx)\n\t"        /* context */
                   "mov " __ASM_NAME("pBTCpuResetToConsistentState") "(%rip),%rax\n\t"
                   "call *%rax\n\t"
                   "add $0x38,%rsp\n\t"
                   "ret" )
#else
void WINAPI Wow64PrepareForException( EXCEPTION_RECORD *rec, CONTEXT *context )
{
    EXCEPTION_POINTERS ptrs = { rec, context };

    pBTCpuResetToConsistentState( &ptrs );
}
#endif


/**********************************************************************
 *           Wow64PassExceptionToGuest  (wow64.@)
 */
void WINAPI Wow64PassExceptionToGuest( EXCEPTION_POINTERS *ptrs )
{
    EXCEPTION_RECORD32 rec32;

    exception_record_64to32( &rec32, ptrs->ExceptionRecord );
    call_user_exception_dispatcher( &rec32, NULL, ptrs->ContextRecord );
}


/**********************************************************************
 *           Wow64ProcessPendingCrossProcessItems  (wow64.@)
 */
void WINAPI Wow64ProcessPendingCrossProcessItems(void)
{
    CROSS_PROCESS_WORK_LIST *list = (void *)wow64info->CrossProcessWorkList;
    CROSS_PROCESS_WORK_ENTRY *entry;
    BOOLEAN flush = FALSE;
    UINT next;

    if (!list) return;
    entry = RtlWow64PopAllCrossProcessWorkFromWorkList( &list->work_list, &flush );

    if (flush)
    {
        if (pBTCpuNotifyFlushInstructionCache2) pBTCpuNotifyFlushInstructionCache2( NULL, ~0ull );
        while (entry)
        {
            next = entry->next;
            RtlWow64PushCrossProcessWorkOntoFreeList( &list->free_list, entry );
            entry = CROSS_PROCESS_LIST_ENTRY( &list->work_list, next );
        }
        return;
    }

    while (entry)
    {
        switch (entry->id)
        {
        case CrossProcessPreVirtualAlloc:
            /* FIXME */
            break;
        case CrossProcessPostVirtualAlloc:
            if (!pBTCpuNotifyMemoryAlloc) break;
            if (entry->args[2]) break;
            pBTCpuNotifyMemoryAlloc( (void *)entry->addr, entry->size, entry->args[0], entry->args[1] );
            break;
        case CrossProcessPreVirtualFree:
            if (!pBTCpuNotifyMemoryFree) break;
            pBTCpuNotifyMemoryFree( (void *)entry->addr, entry->size, entry->args[0] );
            break;
        case CrossProcessPostVirtualFree:
            /* FIXME */
            break;
        case CrossProcessPreVirtualProtect:
            if (!pBTCpuNotifyMemoryProtect) break;
            pBTCpuNotifyMemoryProtect( (void *)entry->addr, entry->size, entry->args[0] );
            break;
        case CrossProcessPostVirtualProtect:
            /* FIXME */
            break;
        case CrossProcessFlushCache:
            if (!pBTCpuNotifyFlushInstructionCache2) break;
            pBTCpuNotifyFlushInstructionCache2( (void *)entry->addr, entry->size );
            break;
        }
        next = entry->next;
        RtlWow64PushCrossProcessWorkOntoFreeList( &list->free_list, entry );
        entry = CROSS_PROCESS_LIST_ENTRY( &list->work_list, next );
    }
}


/**********************************************************************
 *           Wow64RaiseException  (wow64.@)
 */
NTSTATUS WINAPI Wow64RaiseException( int code, EXCEPTION_RECORD *rec )
{
    EXCEPTION_RECORD32 rec32;
    CONTEXT context;
    BOOL first_chance = TRUE;
    union
    {
        I386_CONTEXT i386;
        ARM_CONTEXT arm;
    } ctx32 = { 0 };

    switch (current_machine)
    {
    case IMAGE_FILE_MACHINE_I386:
    {
        EXCEPTION_RECORD int_rec = { 0 };

        ctx32.i386.ContextFlags = CONTEXT_I386_ALL;
        pBTCpuGetContext( GetCurrentThread(), GetCurrentProcess(), NULL, &ctx32.i386 );
        if (code == -1) break;
        int_rec.ExceptionAddress = (void *)(ULONG_PTR)ctx32.i386.Eip;
        switch (code)
        {
        case 0x00:  /* division by zero */
            int_rec.ExceptionCode = EXCEPTION_INT_DIVIDE_BY_ZERO;
            break;
        case 0x01:  /* single-step */
            int_rec.ExceptionCode = EXCEPTION_SINGLE_STEP;
            break;
        case 0x03:  /* breakpoint */
            int_rec.ExceptionCode = EXCEPTION_BREAKPOINT;
            int_rec.ExceptionAddress = (void *)(ULONG_PTR)(ctx32.i386.Eip + 1);
            int_rec.NumberParameters = 1;
            break;
        case 0x04:  /* overflow */
            int_rec.ExceptionCode = EXCEPTION_INT_OVERFLOW;
            break;
        case 0x05:  /* array bounds */
            int_rec.ExceptionCode = EXCEPTION_ARRAY_BOUNDS_EXCEEDED;
            break;
        case 0x06:  /* invalid opcode */
            int_rec.ExceptionCode = EXCEPTION_ILLEGAL_INSTRUCTION;
            break;
        case 0x09:   /* coprocessor segment overrun */
            int_rec.ExceptionCode = EXCEPTION_FLT_INVALID_OPERATION;
            break;
        case 0x0c:  /* stack fault */
            int_rec.ExceptionCode = EXCEPTION_STACK_OVERFLOW;
            break;
        case 0x29:  /* __fastfail */
            int_rec.ExceptionCode = STATUS_STACK_BUFFER_OVERRUN;
            int_rec.ExceptionFlags = EH_NONCONTINUABLE;
            int_rec.NumberParameters = 1;
            int_rec.ExceptionInformation[0] = ctx32.i386.Ecx;
            first_chance = FALSE;
            break;
        case 0x2d:  /* debug service */
            ctx32.i386.Eip++;
            pBTCpuSetContext( GetCurrentThread(), GetCurrentProcess(), NULL, &ctx32.i386 );
            int_rec.ExceptionCode    = EXCEPTION_BREAKPOINT;
            int_rec.ExceptionAddress = (void *)(ULONG_PTR)ctx32.i386.Eip;
            int_rec.NumberParameters = 1;
            int_rec.ExceptionInformation[0] = ctx32.i386.Eax;
            break;
        default:
            ctx32.i386.Eip -= 2;
            pBTCpuSetContext( GetCurrentThread(), GetCurrentProcess(), NULL, &ctx32.i386 );
            int_rec.ExceptionCode = EXCEPTION_ACCESS_VIOLATION;
            int_rec.ExceptionAddress = (void *)(ULONG_PTR)ctx32.i386.Eip;
            int_rec.NumberParameters = 2;
            int_rec.ExceptionInformation[1] = 0xffffffff;
            break;
        }
        *rec = int_rec;
        break;
    }

    case IMAGE_FILE_MACHINE_ARMNT:
        ctx32.arm.ContextFlags = CONTEXT_ARM_ALL;
        pBTCpuGetContext( GetCurrentThread(), GetCurrentProcess(), NULL, &ctx32.arm );
        break;
    }

    exception_record_64to32( &rec32, rec );
    __TRY
    {
        raise_exception( rec, &context, first_chance );
    }
    __EXCEPT_ALL
    {
        call_user_exception_dispatcher( &rec32, &ctx32, NULL );
    }
    __ENDTRY
    return STATUS_SUCCESS;
}
