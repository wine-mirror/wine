/*
 * WoW64 process (and thread) functions
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
#include "wow64_private.h"
#include "wine/asm.h"
#include "wine/exception.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(wow);


static SIZE_T get_machine_context_size( USHORT machine )
{
    switch (machine)
    {
    case IMAGE_FILE_MACHINE_I386:  return sizeof(I386_CONTEXT);
    case IMAGE_FILE_MACHINE_ARMNT: return sizeof(ARM_CONTEXT);
    case IMAGE_FILE_MACHINE_AMD64: return sizeof(AMD64_CONTEXT);
    case IMAGE_FILE_MACHINE_ARM64: return sizeof(ARM64_NT_CONTEXT);
    default: return 0;
    }
}


static BOOL is_process_wow64( HANDLE handle )
{
    ULONG_PTR info;

    if (handle == GetCurrentProcess()) return TRUE;
    if (NtQueryInformationProcess( handle, ProcessWow64Information, &info, sizeof(info), NULL ))
        return FALSE;
    return !!info;
}


static BOOL is_process_id_wow64( const CLIENT_ID *id )
{
    HANDLE handle;
    BOOL ret = FALSE;

    if (id->UniqueProcess == ULongToHandle(GetCurrentProcessId())) return TRUE;
    if (!NtOpenProcess( &handle, PROCESS_QUERY_LIMITED_INFORMATION, NULL, id ))
    {
        ret = is_process_wow64( handle );
        NtClose( handle );
    }
    return ret;
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


static RTL_USER_PROCESS_PARAMETERS *process_params_32to64( RTL_USER_PROCESS_PARAMETERS **params,
                                                           RTL_USER_PROCESS_PARAMETERS32 *params32 )
{
    UNICODE_STRING image, dllpath, curdir, cmdline, title, desktop, shell, runtime;
    RTL_USER_PROCESS_PARAMETERS *ret;

    *params = NULL;
    if (RtlCreateProcessParametersEx( &ret, unicode_str_32to64( &image, &params32->ImagePathName ),
                                      unicode_str_32to64( &dllpath, &params32->DllPath ),
                                      unicode_str_32to64( &curdir, &params32->CurrentDirectory.DosPath ),
                                      unicode_str_32to64( &cmdline, &params32->CommandLine ),
                                      ULongToPtr( params32->Environment ),
                                      unicode_str_32to64( &title, &params32->WindowTitle ),
                                      unicode_str_32to64( &desktop, &params32->Desktop ),
                                      unicode_str_32to64( &shell, &params32->ShellInfo ),
                                      unicode_str_32to64( &runtime, &params32->RuntimeInfo ),
                                      PROCESS_PARAMS_FLAG_NORMALIZED ))
        return NULL;

    ret->DebugFlags            = params32->DebugFlags;
    ret->ConsoleHandle         = LongToHandle( params32->ConsoleHandle );
    ret->ConsoleFlags          = params32->ConsoleFlags;
    ret->hStdInput             = LongToHandle( params32->hStdInput );
    ret->hStdOutput            = LongToHandle( params32->hStdOutput );
    ret->hStdError             = LongToHandle( params32->hStdError );
    ret->dwX                   = params32->dwX;
    ret->dwY                   = params32->dwY;
    ret->dwXSize               = params32->dwXSize;
    ret->dwYSize               = params32->dwYSize;
    ret->dwXCountChars         = params32->dwXCountChars;
    ret->dwYCountChars         = params32->dwYCountChars;
    ret->dwFillAttribute       = params32->dwFillAttribute;
    ret->dwFlags               = params32->dwFlags;
    ret->wShowWindow           = params32->wShowWindow;
    ret->EnvironmentVersion    = params32->EnvironmentVersion;
    ret->PackageDependencyData = ULongToPtr( params32->PackageDependencyData );
    ret->ProcessGroupId        = params32->ProcessGroupId;
    ret->LoaderThreads         = params32->LoaderThreads;
    *params = ret;
    return ret;
}


static void put_ps_create_info( PS_CREATE_INFO32 *info32, const PS_CREATE_INFO *info )
{
    info32->State = info->State;
    switch (info->State)
    {
    case PsCreateInitialState:
        info32->InitState.InitFlags            = info->InitState.InitFlags;
        info32->InitState.AdditionalFileAccess = info->InitState.AdditionalFileAccess;
        break;
    case PsCreateFailOnSectionCreate:
        info32->FailSection.FileHandle = HandleToLong( info->FailSection.FileHandle );
        break;
    case PsCreateFailExeFormat:
        info32->ExeFormat.DllCharacteristics = info->ExeFormat.DllCharacteristics;
        break;
    case PsCreateFailExeName:
        info32->ExeName.IFEOKey = HandleToLong( info->ExeName.IFEOKey );
        break;
    case PsCreateSuccess:
        info32->SuccessState.OutputFlags                 = info->SuccessState.OutputFlags;
        info32->SuccessState.FileHandle                  = HandleToLong( info->SuccessState.FileHandle );
        info32->SuccessState.SectionHandle               = HandleToLong( info->SuccessState.SectionHandle );
        info32->SuccessState.UserProcessParametersNative = info->SuccessState.UserProcessParametersNative;
        info32->SuccessState.UserProcessParametersWow64  = info->SuccessState.UserProcessParametersWow64;
        info32->SuccessState.CurrentParameterFlags       = info->SuccessState.CurrentParameterFlags;
        info32->SuccessState.PebAddressNative            = info->SuccessState.PebAddressNative;
        info32->SuccessState.PebAddressWow64             = info->SuccessState.PebAddressWow64;
        info32->SuccessState.ManifestAddress             = info->SuccessState.ManifestAddress;
        info32->SuccessState.ManifestSize                = info->SuccessState.ManifestSize;
        break;
    default:
        break;
    }
}


static PS_ATTRIBUTE_LIST *ps_attributes_32to64( PS_ATTRIBUTE_LIST **attr, const PS_ATTRIBUTE_LIST32 *attr32 )
{
    PS_ATTRIBUTE_LIST *ret;
    ULONG i, count;

    if (!attr32) return NULL;
    count = (attr32->TotalLength - sizeof(attr32->TotalLength)) / sizeof(PS_ATTRIBUTE32);
    ret = Wow64AllocateTemp( offsetof(PS_ATTRIBUTE_LIST, Attributes[count]) );
    ret->TotalLength = offsetof( PS_ATTRIBUTE_LIST, Attributes[count] );
    for (i = 0; i < count; i++)
    {
        ret->Attributes[i].Attribute    = attr32->Attributes[i].Attribute;
        ret->Attributes[i].Size         = attr32->Attributes[i].Size;
        ret->Attributes[i].Value        = attr32->Attributes[i].Value;
        ret->Attributes[i].ReturnLength = NULL;
        switch (ret->Attributes[i].Attribute)
        {
        case PS_ATTRIBUTE_IMAGE_NAME:
            {
                OBJECT_ATTRIBUTES attr;
                UNICODE_STRING path;

                path.Length = ret->Attributes[i].Size;
                path.Buffer = ret->Attributes[i].ValuePtr;
                InitializeObjectAttributes( &attr, &path, OBJ_CASE_INSENSITIVE, 0, 0 );
                if (get_file_redirect( &attr ))
                {
                    ret->Attributes[i].Size = attr.ObjectName->Length;
                    ret->Attributes[i].ValuePtr = attr.ObjectName->Buffer;
                }
            }
            break;
        case PS_ATTRIBUTE_HANDLE_LIST:
        case PS_ATTRIBUTE_JOB_LIST:
            {
                ULONG j, handles_count = attr32->Attributes[i].Size / sizeof(ULONG);

                ret->Attributes[i].Size     = handles_count * sizeof(HANDLE);
                ret->Attributes[i].ValuePtr = Wow64AllocateTemp( ret->Attributes[i].Size );
                for (j = 0; j < handles_count; j++)
                    ((HANDLE *)ret->Attributes[i].ValuePtr)[j] =
                        LongToHandle( ((LONG *)ULongToPtr(attr32->Attributes[i].Value))[j] );
            }
            break;
        case PS_ATTRIBUTE_PARENT_PROCESS:
            ret->Attributes[i].Size     = sizeof(HANDLE);
            ret->Attributes[i].ValuePtr = LongToHandle( attr32->Attributes[i].Value );
            break;
        case PS_ATTRIBUTE_CLIENT_ID:
            ret->Attributes[i].Size     = sizeof(CLIENT_ID);
            ret->Attributes[i].ValuePtr = Wow64AllocateTemp( ret->Attributes[i].Size );
            break;
        case PS_ATTRIBUTE_IMAGE_INFO:
            ret->Attributes[i].Size     = sizeof(SECTION_IMAGE_INFORMATION);
            ret->Attributes[i].ValuePtr = Wow64AllocateTemp( ret->Attributes[i].Size );
            break;
        case PS_ATTRIBUTE_TEB_ADDRESS:
            ret->Attributes[i].Size     = sizeof(TEB *);
            ret->Attributes[i].ValuePtr = Wow64AllocateTemp( ret->Attributes[i].Size );
            break;
        }
    }
    *attr = ret;
    return ret;
}


static void put_ps_attributes( PS_ATTRIBUTE_LIST32 *attr32, const PS_ATTRIBUTE_LIST *attr )
{
    ULONG i;

    if (!attr32) return;
    for (i = 0; i < (attr32->TotalLength - sizeof(attr32->TotalLength)) / sizeof(PS_ATTRIBUTE32); i++)
    {
        switch (attr->Attributes[i].Attribute)
        {
        case PS_ATTRIBUTE_CLIENT_ID:
        {
            CLIENT_ID32 id32;
            ULONG size = min( attr32->Attributes[i].Size, sizeof(id32) );
            put_client_id( &id32, attr->Attributes[i].ValuePtr );
            memcpy( ULongToPtr( attr32->Attributes[i].Value ), &id32, size );
            if (attr32->Attributes[i].ReturnLength)
                *(ULONG *)ULongToPtr(attr32->Attributes[i].ReturnLength) = size;
            break;
        }
        case PS_ATTRIBUTE_IMAGE_INFO:
        {
            SECTION_IMAGE_INFORMATION32 info32;
            ULONG size = min( attr32->Attributes[i].Size, sizeof(info32) );
            put_section_image_info( &info32, attr->Attributes[i].ValuePtr );
            memcpy( ULongToPtr( attr32->Attributes[i].Value ), &info32, size );
            if (attr32->Attributes[i].ReturnLength)
                *(ULONG *)ULongToPtr(attr32->Attributes[i].ReturnLength) = size;
            break;
        }
        case PS_ATTRIBUTE_TEB_ADDRESS:
        {
            TEB **teb = attr->Attributes[i].ValuePtr;
            ULONG teb32 = PtrToUlong( *teb ) + 0x2000;
            ULONG size = min( attr->Attributes[i].Size, sizeof(teb32) );
            memcpy( ULongToPtr( attr32->Attributes[i].Value ), &teb32, size );
            if (attr32->Attributes[i].ReturnLength)
                *(ULONG *)ULongToPtr(attr32->Attributes[i].ReturnLength) = size;
            break;
        }
        }
    }
}


void put_vm_counters( VM_COUNTERS_EX32 *info32, const VM_COUNTERS_EX *info, ULONG size )
{
    info32->PeakVirtualSize            = info->PeakVirtualSize;
    info32->VirtualSize                = info->VirtualSize;
    info32->PageFaultCount             = info->PageFaultCount;
    info32->PeakWorkingSetSize         = info->PeakWorkingSetSize;
    info32->WorkingSetSize             = info->WorkingSetSize;
    info32->QuotaPeakPagedPoolUsage    = info->QuotaPeakPagedPoolUsage;
    info32->QuotaPagedPoolUsage        = info->QuotaPagedPoolUsage;
    info32->QuotaPeakNonPagedPoolUsage = info->QuotaPeakNonPagedPoolUsage;
    info32->QuotaNonPagedPoolUsage     = info->QuotaNonPagedPoolUsage;
    info32->PagefileUsage              = info->PagefileUsage;
    info32->PeakPagefileUsage          = info->PeakPagefileUsage;
    if (size == sizeof(VM_COUNTERS_EX32)) info32->PrivateUsage = info->PrivateUsage;
}


static void call_user_exception_dispatcher( EXCEPTION_RECORD32 *rec, void *ctx32_ptr, void *ctx64_ptr )
{
    switch (current_machine)
    {
    case IMAGE_FILE_MACHINE_I386:
        {
            struct stack_layout
            {
                ULONG               rec_ptr;       /* first arg for KiUserExceptionDispatcher */
                ULONG               context_ptr;   /* second arg for KiUserExceptionDispatcher */
                EXCEPTION_RECORD32  rec;
                I386_CONTEXT        context;
            } *stack;
            I386_CONTEXT *context, ctx = { CONTEXT_I386_ALL };
            CONTEXT_EX *context_ex, *src_ex = NULL;
            ULONG size, flags;

            NtQueryInformationThread( GetCurrentThread(), ThreadWow64Context, &ctx, sizeof(ctx), NULL );

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
            RtlGetExtendedContextLength( flags, &size );
            size = ((size + 15) & ~15) + offsetof(struct stack_layout,context);

            stack = (struct stack_layout *)(ULONG_PTR)(ctx.Esp - size);
            stack->rec_ptr = PtrToUlong( &stack->rec );
            stack->rec = *rec;
            RtlInitializeExtendedContext( &stack->context, flags, &context_ex );
            context = RtlLocateLegacyContext( context_ex, NULL );
            *context = ctx;
            context->ContextFlags = flags;
            stack->context_ptr = PtrToUlong( context );

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
            NtSetInformationThread( GetCurrentThread(), ThreadWow64Context, &ctx, sizeof(ctx) );

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

            NtQueryInformationThread( GetCurrentThread(), ThreadWow64Context, &ctx, sizeof(ctx), NULL );
            stack = (struct stack_layout *)(ULONG_PTR)(ctx.Sp & ~3) - 1;
            stack->rec = *rec;
            stack->context = ctx;

            ctx.R0 = PtrToUlong( &stack->rec );     /* first arg for KiUserExceptionDispatcher */
            ctx.R1 = PtrToUlong( &stack->context ); /* second arg for KiUserExceptionDispatcher */
            ctx.Sp = PtrToUlong( stack );
            ctx.Pc = pLdrSystemDllInitBlock->pKiUserExceptionDispatcher;
            if (ctx.Pc & 1) ctx.Cpsr |= 0x20;
            else ctx.Cpsr &= ~0x20;
            NtSetInformationThread( GetCurrentThread(), ThreadWow64Context, &ctx, sizeof(ctx) );

            TRACE( "exception %08lx dispatcher %08lx stack %08lx pc %08lx\n",
                   rec->ExceptionCode, ctx.Pc, ctx.Sp, stack->context.Sp );
        }
        break;
    }
}


/* based on RtlRaiseException: call NtRaiseException with context setup to return to caller */
void WINAPI raise_exception( EXCEPTION_RECORD *rec, CONTEXT *context, BOOL first_chance ) DECLSPEC_HIDDEN;
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
                   "movq %rax,0x10(%rcx)\n\t"   /* rec->ExceptionAddress */
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
                   "str x5, [x0, #0x10]\n\t"      /* rec->ExceptionAddress */
                   "bl " __ASM_NAME("NtRaiseException") )
#endif


/**********************************************************************
 *           wow64_NtAlertResumeThread
 */
NTSTATUS WINAPI wow64_NtAlertResumeThread( UINT *args )
{
    HANDLE handle = get_handle( &args );
    ULONG *count = get_ptr( &args );

    return NtAlertResumeThread( handle, count );
}


/**********************************************************************
 *           wow64_NtAlertThread
 */
NTSTATUS WINAPI wow64_NtAlertThread( UINT *args )
{
    HANDLE handle = get_handle( &args );

    return NtAlertThread( handle );
}


/**********************************************************************
 *           wow64_NtAlertThreadByThreadId
 */
NTSTATUS WINAPI wow64_NtAlertThreadByThreadId( UINT *args )
{
    HANDLE tid = get_handle( &args );

    return NtAlertThreadByThreadId( tid );
}


/**********************************************************************
 *           wow64_NtAssignProcessToJobObject
 */
NTSTATUS WINAPI wow64_NtAssignProcessToJobObject( UINT *args )
{
    HANDLE job = get_handle( &args );
    HANDLE process = get_handle( &args );

    return NtAssignProcessToJobObject( job, process );
}


/**********************************************************************
 *           wow64_NtContinue
 */
NTSTATUS WINAPI wow64_NtContinue( UINT *args )
{
    void *context = get_ptr( &args );
    BOOLEAN alertable = get_ulong( &args );

    NtSetInformationThread( GetCurrentThread(), ThreadWow64Context,
                            context, get_machine_context_size( current_machine ));
    if (alertable) NtTestAlert();
    return STATUS_SUCCESS;
}


/**********************************************************************
 *           wow64_NtCreateThread
 */
NTSTATUS WINAPI wow64_NtCreateThread( UINT *args )
{
    ULONG *handle_ptr = get_ptr( &args );
    ACCESS_MASK access = get_ulong( &args );
    OBJECT_ATTRIBUTES32 *attr32 = get_ptr( &args );
    HANDLE process = get_handle( &args );
    CLIENT_ID32 *id32 = get_ptr( &args );
    I386_CONTEXT *context = get_ptr( &args );
    void *initial_teb = get_ptr( &args );
    BOOLEAN suspended = get_ulong( &args );

    FIXME( "%p %lx %p %p %p %p %p %u: stub\n", handle_ptr, access, attr32, process,
           id32, context, initial_teb, suspended );
    return STATUS_NOT_IMPLEMENTED;
}


/**********************************************************************
 *           wow64_NtCreateThreadEx
 */
NTSTATUS WINAPI wow64_NtCreateThreadEx( UINT *args )
{
    ULONG *handle_ptr = get_ptr( &args );
    ACCESS_MASK access = get_ulong( &args );
    OBJECT_ATTRIBUTES32 *attr32 = get_ptr( &args );
    HANDLE process = get_handle( &args );
    PRTL_THREAD_START_ROUTINE start = get_ptr( &args );
    void *param = get_ptr( &args );
    ULONG flags = get_ulong( &args );
    ULONG_PTR zero_bits = get_ulong( &args );
    SIZE_T stack_commit = get_ulong( &args );
    SIZE_T stack_reserve = get_ulong( &args );
    PS_ATTRIBUTE_LIST32 *attr_list32 = get_ptr( &args );

    struct object_attr64 attr;
    PS_ATTRIBUTE_LIST *attr_list;
    HANDLE handle = 0;
    NTSTATUS status;

    *handle_ptr = 0;
    if (is_process_wow64( process ))
    {
        status = NtCreateThreadEx( &handle, access, objattr_32to64( &attr, attr32 ), process,
                                   start, param, flags, get_zero_bits( zero_bits ),
                                   stack_commit, stack_reserve,
                                   ps_attributes_32to64( &attr_list, attr_list32 ));
        put_ps_attributes( attr_list32, attr_list );
    }
    else status = STATUS_ACCESS_DENIED;

    put_handle( handle_ptr, handle );
    return status;
}


/**********************************************************************
 *           wow64_NtCreateUserProcess
 */
NTSTATUS WINAPI wow64_NtCreateUserProcess( UINT *args )
{
    ULONG *process_handle_ptr = get_ptr( &args );
    ULONG *thread_handle_ptr = get_ptr( &args );
    ACCESS_MASK process_access = get_ulong( &args );
    ACCESS_MASK thread_access = get_ulong( &args );
    OBJECT_ATTRIBUTES32 *process_attr32 = get_ptr( &args );
    OBJECT_ATTRIBUTES32 *thread_attr32 = get_ptr( &args );
    ULONG process_flags = get_ulong( &args );
    ULONG thread_flags = get_ulong( &args );
    RTL_USER_PROCESS_PARAMETERS32 *params32 = get_ptr( &args );
    PS_CREATE_INFO32 *info32 = get_ptr( &args );
    PS_ATTRIBUTE_LIST32 *attr32 = get_ptr( &args );

    struct object_attr64 process_attr, thread_attr;
    RTL_USER_PROCESS_PARAMETERS *params;
    PS_CREATE_INFO info;
    PS_ATTRIBUTE_LIST *attr;
    HANDLE process_handle = 0, thread_handle = 0;

    NTSTATUS status;

    *process_handle_ptr = *thread_handle_ptr = 0;
    status = NtCreateUserProcess( &process_handle, &thread_handle, process_access, thread_access,
                                  objattr_32to64( &process_attr, process_attr32 ),
                                  objattr_32to64( &thread_attr, thread_attr32 ),
                                  process_flags, thread_flags,
                                  process_params_32to64( &params, params32),
                                  &info, ps_attributes_32to64( &attr, attr32 ));
    put_handle( process_handle_ptr, process_handle );
    put_handle( thread_handle_ptr, thread_handle );
    put_ps_create_info( info32, &info );
    put_ps_attributes( attr32, attr );
    RtlDestroyProcessParameters( params );
    return status;
}


/**********************************************************************
 *           wow64_NtDebugActiveProcess
 */
NTSTATUS WINAPI wow64_NtDebugActiveProcess( UINT *args )
{
    HANDLE process = get_handle( &args );
    HANDLE debug = get_handle( &args );

    return NtDebugActiveProcess( process, debug );
}


/**********************************************************************
 *           wow64_NtFlushInstructionCache
 */
NTSTATUS WINAPI wow64_NtFlushInstructionCache( UINT *args )
{
    HANDLE process = get_handle( &args );
    const void *addr = get_ptr( &args );
    SIZE_T size = get_ulong( &args );

    return NtFlushInstructionCache( process, addr, size );
}


/**********************************************************************
 *           wow64_NtFlushProcessWriteBuffers
 */
NTSTATUS WINAPI wow64_NtFlushProcessWriteBuffers( UINT *args )
{
    NtFlushProcessWriteBuffers();
    return STATUS_SUCCESS;
}


/**********************************************************************
 *           wow64_NtGetContextThread
 */
NTSTATUS WINAPI wow64_NtGetContextThread( UINT *args )
{
    HANDLE handle = get_handle( &args );
    WOW64_CONTEXT *context = get_ptr( &args );

    return NtQueryInformationThread( handle, ThreadWow64Context, context,
                                     get_machine_context_size( current_machine ), NULL );
}


/**********************************************************************
 *           wow64_NtGetNextThread
 */
NTSTATUS WINAPI wow64_NtGetNextThread( UINT *args )
{
    HANDLE process = get_handle( &args );
    HANDLE thread = get_handle( &args );
    ACCESS_MASK access = get_ulong( &args );
    ULONG attributes = get_ulong( &args );
    ULONG flags = get_ulong( &args );
    ULONG *handle_ptr = get_ptr( &args );

    HANDLE handle = 0;
    NTSTATUS status;

    *handle_ptr = 0;
    status = NtGetNextThread( process, thread, access, attributes, flags, &handle );
    put_handle( handle_ptr, handle );
    return status;
}


/**********************************************************************
 *           wow64_NtIsProcessInJob
 */
NTSTATUS WINAPI wow64_NtIsProcessInJob( UINT *args )
{
    HANDLE process = get_handle( &args );
    HANDLE job = get_handle( &args );

    return NtIsProcessInJob( process, job );
}


/**********************************************************************
 *           wow64_NtOpenProcess
 */
NTSTATUS WINAPI wow64_NtOpenProcess( UINT *args )
{
    ULONG *handle_ptr = get_ptr( &args );
    ACCESS_MASK access = get_ulong( &args );
    OBJECT_ATTRIBUTES32 *attr32 = get_ptr( &args );
    CLIENT_ID32 *id32 = get_ptr( &args );

    struct object_attr64 attr;
    HANDLE handle = 0;
    CLIENT_ID id;
    NTSTATUS status;

    *handle_ptr = 0;
    status = NtOpenProcess( &handle, access, objattr_32to64( &attr, attr32 ), client_id_32to64( &id, id32 ));
    put_handle( handle_ptr, handle );
    return status;
}


/**********************************************************************
 *           wow64_NtOpenThread
 */
NTSTATUS WINAPI wow64_NtOpenThread( UINT *args )
{
    ULONG *handle_ptr = get_ptr( &args );
    ACCESS_MASK access = get_ulong( &args );
    OBJECT_ATTRIBUTES32 *attr32 = get_ptr( &args );
    CLIENT_ID32 *id32 = get_ptr( &args );

    struct object_attr64 attr;
    HANDLE handle = 0;
    CLIENT_ID id;
    NTSTATUS status;

    *handle_ptr = 0;
    status = NtOpenThread( &handle, access, objattr_32to64( &attr, attr32 ), client_id_32to64( &id, id32 ));
    put_handle( handle_ptr, handle );
    return status;
}


/**********************************************************************
 *           wow64_NtQueryInformationProcess
 */
NTSTATUS WINAPI wow64_NtQueryInformationProcess( UINT *args )
{
    HANDLE handle = get_handle( &args );
    PROCESSINFOCLASS class = get_ulong( &args );
    void *ptr = get_ptr( &args );
    ULONG len = get_ulong( &args );
    ULONG *retlen = get_ptr( &args );

    NTSTATUS status;

    switch (class)
    {
    case ProcessBasicInformation:  /* PROCESS_BASIC_INFORMATION */
        if (len == sizeof(PROCESS_BASIC_INFORMATION32))
        {
            PROCESS_BASIC_INFORMATION info;
            PROCESS_BASIC_INFORMATION32 *info32 = ptr;

            if (!(status = NtQueryInformationProcess( handle, class, &info, sizeof(info), NULL )))
            {
                if (is_process_wow64( handle ))
                    info32->PebBaseAddress = PtrToUlong( info.PebBaseAddress ) + 0x1000;
                else
                    info32->PebBaseAddress = 0;
                info32->ExitStatus = info.ExitStatus;
                info32->AffinityMask = info.AffinityMask;
                info32->BasePriority = info.BasePriority;
                info32->UniqueProcessId = info.UniqueProcessId;
                info32->InheritedFromUniqueProcessId = info.InheritedFromUniqueProcessId;
                if (retlen) *retlen = sizeof(*info32);
            }
            return status;
        }
        if (retlen) *retlen = sizeof(PROCESS_BASIC_INFORMATION32);
        return STATUS_INFO_LENGTH_MISMATCH;

    case ProcessIoCounters:  /* IO_COUNTERS */
    case ProcessTimes:  /* KERNEL_USER_TIMES */
    case ProcessDefaultHardErrorMode:  /* ULONG */
    case ProcessPriorityClass:  /* PROCESS_PRIORITY_CLASS */
    case ProcessHandleCount:  /* ULONG */
    case ProcessSessionInformation:  /* ULONG */
    case ProcessDebugFlags:  /* ULONG */
    case ProcessExecuteFlags:  /* ULONG */
    case ProcessCookie:  /* ULONG */
        /* FIXME: check buffer alignment */
        return NtQueryInformationProcess( handle, class, ptr, len, retlen );

    case ProcessVmCounters:  /* VM_COUNTERS_EX */
        if (len == sizeof(VM_COUNTERS32) || len == sizeof(VM_COUNTERS_EX32))
        {
            VM_COUNTERS_EX info;
            VM_COUNTERS_EX32 *info32 = ptr;

            if (!(status = NtQueryInformationProcess( handle, class, &info, sizeof(info), NULL )))
            {
                put_vm_counters( info32, &info, len );
                if (retlen) *retlen = len;
            }
            return status;
        }
        if (retlen) *retlen = sizeof(VM_COUNTERS_EX32);
        return STATUS_INFO_LENGTH_MISMATCH;

    case ProcessDebugPort:  /* ULONG_PTR */
    case ProcessAffinityMask:  /* ULONG_PTR */
    case ProcessWow64Information:  /* ULONG_PTR */
    case ProcessDebugObjectHandle:  /* HANDLE */
        if (len == sizeof(ULONG))
        {
            ULONG_PTR data;

            if (!(status = NtQueryInformationProcess( handle, class, &data, sizeof(data), NULL )))
            {
                *(ULONG *)ptr = data;
                if (retlen) *retlen = sizeof(ULONG);
            }
            else if (status == STATUS_PORT_NOT_SET) *(ULONG *)ptr = 0;
            return status;
        }
        if (retlen) *retlen = sizeof(ULONG);
        return STATUS_INFO_LENGTH_MISMATCH;

    case ProcessImageFileName:
    case ProcessImageFileNameWin32:  /* UNICODE_STRING + string */
        {
            ULONG retsize, size = len + sizeof(UNICODE_STRING) - sizeof(UNICODE_STRING32);
            UNICODE_STRING *str = Wow64AllocateTemp( size );
            UNICODE_STRING32 *str32 = ptr;

            if (!(status = NtQueryInformationProcess( handle, class, str, size, &retsize )))
            {
                str32->Length = str->Length;
                str32->MaximumLength = str->MaximumLength;
                str32->Buffer = PtrToUlong( str32 + 1 );
                memcpy( str32 + 1, str->Buffer, str->MaximumLength );
            }
            if (retlen) *retlen = retsize + sizeof(UNICODE_STRING32) - sizeof(UNICODE_STRING);
            return status;
        }

    case ProcessImageInformation:  /* SECTION_IMAGE_INFORMATION */
        if (len == sizeof(SECTION_IMAGE_INFORMATION32))
        {
            SECTION_IMAGE_INFORMATION info;
            SECTION_IMAGE_INFORMATION32 *info32 = ptr;

            if (!(status = NtQueryInformationProcess( handle, class, &info, sizeof(info), NULL )))
            {
                put_section_image_info( info32, &info );
                if (retlen) *retlen = sizeof(*info32);
            }
            return status;
        }
        if (retlen) *retlen = sizeof(SECTION_IMAGE_INFORMATION32);
        return STATUS_INFO_LENGTH_MISMATCH;

    default:
        FIXME( "unsupported class %u\n", class );
        return STATUS_INVALID_INFO_CLASS;
    }
}


/**********************************************************************
 *           wow64_NtQueryInformationThread
 */
NTSTATUS WINAPI wow64_NtQueryInformationThread( UINT *args )
{
    HANDLE handle = get_handle( &args );
    THREADINFOCLASS class = get_ulong( &args );
    void *ptr = get_ptr( &args );
    ULONG len = get_ulong( &args );
    ULONG *retlen = get_ptr( &args );

    NTSTATUS status;

    switch (class)
    {
    case ThreadBasicInformation:  /* THREAD_BASIC_INFORMATION */
    {
        THREAD_BASIC_INFORMATION32 info32;
        THREAD_BASIC_INFORMATION info;

        status = NtQueryInformationThread( handle, class, &info, sizeof(info), NULL );
        if (!status)
        {
            info32.ExitStatus = info.ExitStatus;
            info32.TebBaseAddress = is_process_id_wow64( &info.ClientId ) ?
                                    PtrToUlong(info.TebBaseAddress) + 0x2000 : 0;
            info32.ClientId.UniqueProcess = HandleToULong( info.ClientId.UniqueProcess );
            info32.ClientId.UniqueThread = HandleToULong( info.ClientId.UniqueThread );
            info32.AffinityMask = info.AffinityMask;
            info32.Priority = info.Priority;
            info32.BasePriority = info.BasePriority;
            memcpy( ptr, &info32, min( len, sizeof(info32) ));
            if (retlen) *retlen = min( len, sizeof(info32) );
        }
        return status;
    }

    case ThreadTimes:  /* KERNEL_USER_TIMES */
    case ThreadEnableAlignmentFaultFixup:  /* set only */
    case ThreadAmILastThread:  /* ULONG */
    case ThreadIsIoPending:  /* ULONG */
    case ThreadHideFromDebugger:  /* BOOLEAN */
    case ThreadSuspendCount:  /* ULONG */
        /* FIXME: check buffer alignment */
        return NtQueryInformationThread( handle, class, ptr, len, retlen );

    case ThreadAffinityMask:  /* ULONG_PTR */
    case ThreadQuerySetWin32StartAddress:  /* PRTL_THREAD_START_ROUTINE */
    {
        ULONG_PTR data;

        status = NtQueryInformationThread( handle, class, &data, sizeof(data), NULL );
        if (!status)
        {
            memcpy( ptr, &data, min( len, sizeof(ULONG) ));
            if (retlen) *retlen = min( len, sizeof(ULONG) );
        }
        return status;
    }

    case ThreadDescriptorTableEntry:  /* THREAD_DESCRIPTOR_INFORMATION */
        return RtlWow64GetThreadSelectorEntry( handle, ptr, len, retlen );

    case ThreadWow64Context:  /* WOW64_CONTEXT* */
        return STATUS_INVALID_INFO_CLASS;

    case ThreadGroupInformation:  /* GROUP_AFFINITY */
    {
        GROUP_AFFINITY info;

        status = NtQueryInformationThread( handle, class, &info, sizeof(info), NULL );
        if (!status)
        {
            GROUP_AFFINITY32 info32 = { info.Mask, info.Group };
            memcpy( ptr, &info32, min( len, sizeof(info32) ));
            if (retlen) *retlen = min( len, sizeof(info32) );
        }
        return status;
    }

    case ThreadNameInformation:  /* THREAD_NAME_INFORMATION */
    {
        THREAD_NAME_INFORMATION *info;
        THREAD_NAME_INFORMATION32 *info32 = ptr;
        ULONG size, ret_size;

        if (len >= sizeof(*info32))
        {
            size = sizeof(*info) + len - sizeof(*info32);
            info = Wow64AllocateTemp( size );
            status = NtQueryInformationThread( handle, class, info, size, &ret_size );
            if (!status)
            {
                info32->ThreadName.Length = info->ThreadName.Length;
                info32->ThreadName.MaximumLength = info->ThreadName.MaximumLength;
                info32->ThreadName.Buffer = PtrToUlong( info32 + 1 );
                memcpy( info32 + 1, info + 1, min( len, info->ThreadName.MaximumLength ));
            }
        }
        else status = NtQueryInformationThread( handle, class, NULL, 0, &ret_size );

        if (retlen && (status == STATUS_SUCCESS || status == STATUS_BUFFER_TOO_SMALL))
            *retlen = sizeof(*info32) + ret_size - sizeof(*info);
        return status;
    }

    default:
        FIXME( "unsupported class %u\n", class );
        return STATUS_INVALID_INFO_CLASS;
    }
}


/**********************************************************************
 *           wow64_NtQueueApcThread
 */
NTSTATUS WINAPI wow64_NtQueueApcThread( UINT *args )
{
    HANDLE handle = get_handle( &args );
    ULONG func = get_ulong( &args );
    ULONG arg1 = get_ulong( &args );
    ULONG arg2 = get_ulong( &args );
    ULONG arg3 = get_ulong( &args );

    return NtQueueApcThread( handle, apc_32to64( func ),
                             (ULONG_PTR)apc_param_32to64( func, arg1 ), arg2, arg3 );
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

    NtSetInformationThread( GetCurrentThread(), ThreadWow64Context,
                            context32, get_machine_context_size( current_machine ));

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
 *           wow64_NtRemoveProcessDebug
 */
NTSTATUS WINAPI wow64_NtRemoveProcessDebug( UINT *args )
{
    HANDLE process = get_handle( &args );
    HANDLE debug = get_handle( &args );

    return NtRemoveProcessDebug( process, debug );
}


/**********************************************************************
 *           wow64_NtResumeProcess
 */
NTSTATUS WINAPI wow64_NtResumeProcess( UINT *args )
{
    HANDLE handle = get_handle( &args );

    return NtResumeProcess( handle );
}


/**********************************************************************
 *           wow64_NtResumeThread
 */
NTSTATUS WINAPI wow64_NtResumeThread( UINT *args )
{
    HANDLE handle = get_handle( &args );
    ULONG *count = get_ptr( &args );

    return NtResumeThread( handle, count );
}


/**********************************************************************
 *           wow64_NtSetContextThread
 */
NTSTATUS WINAPI wow64_NtSetContextThread( UINT *args )
{
    HANDLE handle = get_handle( &args );
    WOW64_CONTEXT *context = get_ptr( &args );

    return NtSetInformationThread( handle, ThreadWow64Context,
                                   context, get_machine_context_size( current_machine ));
}


/**********************************************************************
 *           wow64_NtSetInformationProcess
 */
NTSTATUS WINAPI wow64_NtSetInformationProcess( UINT *args )
{
    HANDLE handle = get_handle( &args );
    PROCESSINFOCLASS class = get_ulong( &args );
    void *ptr = get_ptr( &args );
    ULONG len = get_ulong( &args );

    NTSTATUS status;

    switch (class)
    {
    case ProcessDefaultHardErrorMode:   /* ULONG */
    case ProcessPriorityClass:   /* PROCESS_PRIORITY_CLASS */
    case ProcessExecuteFlags:   /* ULONG */
        return NtSetInformationProcess( handle, class, ptr, len );

    case ProcessAffinityMask:   /* ULONG_PTR */
        if (len == sizeof(ULONG))
        {
            ULONG_PTR mask = *(ULONG *)ptr;
            return NtSetInformationProcess( handle, class, &mask, sizeof(mask) );
        }
        else return STATUS_INVALID_PARAMETER;

    case ProcessInstrumentationCallback:   /* PROCESS_INSTRUMENTATION_CALLBACK_INFORMATION */
        if (len == sizeof(PROCESS_INSTRUMENTATION_CALLBACK_INFORMATION32))
        {
            FIXME( "ProcessInstrumentationCallback stub\n" );
            return STATUS_SUCCESS;
        }
        else return STATUS_INFO_LENGTH_MISMATCH;

    case ProcessThreadStackAllocation:   /* PROCESS_STACK_ALLOCATION_INFORMATION(_EX) */
        if (len == sizeof(PROCESS_STACK_ALLOCATION_INFORMATION_EX32))
        {
            PROCESS_STACK_ALLOCATION_INFORMATION_EX32 *stack = ptr;
            PROCESS_STACK_ALLOCATION_INFORMATION_EX info;

            info.PreferredNode = stack->PreferredNode;
            info.Reserved0 = stack->Reserved0;
            info.Reserved1 = stack->Reserved1;
            info.Reserved2 = stack->Reserved2;
            info.AllocInfo.ReserveSize = stack->AllocInfo.ReserveSize;
            info.AllocInfo.ZeroBits = get_zero_bits( stack->AllocInfo.ZeroBits );
            if (!(status = NtSetInformationProcess( handle, class, &info, sizeof(info) )))
                stack->AllocInfo.StackBase = PtrToUlong( info.AllocInfo.StackBase );
            return status;
        }
        else if (len == sizeof(PROCESS_STACK_ALLOCATION_INFORMATION32))
        {
            PROCESS_STACK_ALLOCATION_INFORMATION32 *stack = ptr;
            PROCESS_STACK_ALLOCATION_INFORMATION info;

            info.ReserveSize = stack->ReserveSize;
            info.ZeroBits = get_zero_bits( stack->ZeroBits );
            if (!(status = NtSetInformationProcess( handle, class, &info, sizeof(info) )))
                stack->StackBase = PtrToUlong( info.StackBase );
            return status;
        }
        else return STATUS_INFO_LENGTH_MISMATCH;

    case ProcessWineMakeProcessSystem:   /* HANDLE* */
        if (len == sizeof(ULONG))
        {
            HANDLE event = 0;
            status = NtSetInformationProcess( handle, class, &event, sizeof(HANDLE *) );
            put_handle( ptr, event );
            return status;
        }
        else return STATUS_INFO_LENGTH_MISMATCH;

    default:
        FIXME( "unsupported class %u\n", class );
        return STATUS_INVALID_INFO_CLASS;
    }
}


/**********************************************************************
 *           wow64_NtSetInformationThread
 */
NTSTATUS WINAPI wow64_NtSetInformationThread( UINT *args )
{
    HANDLE handle = get_handle( &args );
    THREADINFOCLASS class = get_ulong( &args );
    void *ptr = get_ptr( &args );
    ULONG len = get_ulong( &args );

    switch (class)
    {
    case ThreadZeroTlsCell:   /* ULONG */
    case ThreadBasePriority:   /* ULONG */
    case ThreadHideFromDebugger:   /* void */
    case ThreadEnableAlignmentFaultFixup:   /* BOOLEAN */
    case ThreadPowerThrottlingState:  /* THREAD_POWER_THROTTLING_STATE */
        return NtSetInformationThread( handle, class, ptr, len );

    case ThreadImpersonationToken:   /* HANDLE */
        if (len == sizeof(ULONG))
        {
            HANDLE token = LongToHandle( *(ULONG *)ptr );
            return NtSetInformationThread( handle, class, &token, sizeof(token) );
        }
        else return STATUS_INVALID_PARAMETER;

    case ThreadAffinityMask:  /* ULONG_PTR */
    case ThreadQuerySetWin32StartAddress:   /* PRTL_THREAD_START_ROUTINE */
        if (len == sizeof(ULONG))
        {
            ULONG_PTR mask = *(ULONG *)ptr;
            return NtSetInformationThread( handle, class, &mask, sizeof(mask) );
        }
        else return STATUS_INVALID_PARAMETER;

    case ThreadWow64Context:  /* WOW64_CONTEXT* */
        return STATUS_INVALID_INFO_CLASS;

    case ThreadGroupInformation:   /* GROUP_AFFINITY */
        if (len == sizeof(GROUP_AFFINITY32))
        {
            GROUP_AFFINITY32 *info32 = ptr;
            GROUP_AFFINITY info = { info32->Mask, info32->Group };

            return NtSetInformationThread( handle, class, &info, sizeof(info) );
        }
        else return STATUS_INVALID_PARAMETER;

    case ThreadNameInformation:   /* THREAD_NAME_INFORMATION */
        if (len == sizeof(THREAD_NAME_INFORMATION32))
        {
            THREAD_NAME_INFORMATION32 *info32 = ptr;
            THREAD_NAME_INFORMATION info;

            if (!unicode_str_32to64( &info.ThreadName, &info32->ThreadName ))
                return STATUS_ACCESS_VIOLATION;
            return NtSetInformationThread( handle, class, &info, sizeof(info) );
        }
        else return STATUS_INFO_LENGTH_MISMATCH;

    default:
        FIXME( "unsupported class %u\n", class );
        return STATUS_INVALID_INFO_CLASS;
    }
}


/**********************************************************************
 *           wow64_NtSetThreadExecutionState
 */
NTSTATUS WINAPI wow64_NtSetThreadExecutionState( UINT *args )
{
    EXECUTION_STATE new_state = get_ulong( &args );
    EXECUTION_STATE *old_state = get_ptr( &args );

    return NtSetThreadExecutionState( new_state, old_state );
}


/**********************************************************************
 *           wow64_NtSuspendProcess
 */
NTSTATUS WINAPI wow64_NtSuspendProcess( UINT *args )
{
    HANDLE handle = get_handle( &args );

    return NtSuspendProcess( handle );
}


/**********************************************************************
 *           wow64_NtSuspendThread
 */
NTSTATUS WINAPI wow64_NtSuspendThread( UINT *args )
{
    HANDLE handle = get_handle( &args );
    ULONG *count = get_ptr( &args );

    return NtSuspendThread( handle, count );
}


/**********************************************************************
 *           wow64_NtTerminateProcess
 */
NTSTATUS WINAPI wow64_NtTerminateProcess( UINT *args )
{
    HANDLE handle = get_handle( &args );
    LONG exit_code = get_ulong( &args );

    return NtTerminateProcess( handle, exit_code );
}


/**********************************************************************
 *           wow64_NtTerminateThread
 */
NTSTATUS WINAPI wow64_NtTerminateThread( UINT *args )
{
    HANDLE handle = get_handle( &args );
    LONG exit_code = get_ulong( &args );

    return NtTerminateThread( handle, exit_code );
}


/**********************************************************************
 *           Wow64PassExceptionToGuest  (wow64.@)
 */
void WINAPI Wow64PassExceptionToGuest( EXCEPTION_POINTERS *ptrs )
{
    EXCEPTION_RECORD *rec = ptrs->ExceptionRecord;
    EXCEPTION_RECORD32 rec32;
    ULONG i;

    rec32.ExceptionCode    = rec->ExceptionCode;
    rec32.ExceptionFlags   = rec->ExceptionFlags;
    rec32.ExceptionRecord  = PtrToUlong( rec->ExceptionRecord );
    rec32.ExceptionAddress = PtrToUlong( rec->ExceptionAddress );
    rec32.NumberParameters = rec->NumberParameters;
    for (i = 0; i < rec->NumberParameters; i++)
        rec32.ExceptionInformation[i] = rec->ExceptionInformation[i];

    call_user_exception_dispatcher( &rec32, NULL, ptrs->ContextRecord );
}
