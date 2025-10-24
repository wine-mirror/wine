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
#include "ddk/wdm.h"
#include "wine/exception.h"
#include "wine/list.h"
#include "ntdll_misc.h"
#include "unwind.h"
#include "wine/debug.h"
#include "ntsyscalls.h"

WINE_DEFAULT_DEBUG_CHANNEL(seh);
WINE_DECLARE_DEBUG_CHANNEL(relay);

/* xtajit64.dll functions */
static void     (WINAPI *pBTCpu64FlushInstructionCache)(const void*,SIZE_T);
static BOOLEAN  (WINAPI *pBTCpu64IsProcessorFeaturePresent)(UINT);
static void     (WINAPI *pBTCpu64NotifyMemoryDirty)(void*,SIZE_T);
static void     (WINAPI *pBTCpu64NotifyReadFile)(HANDLE,void*,SIZE_T,BOOL,NTSTATUS);
static void     (WINAPI *pBeginSimulation)(void);
static void     (WINAPI *pFlushInstructionCacheHeavy)(const void*,SIZE_T);
static NTSTATUS (WINAPI *pNotifyMapViewOfSection)(void*,void*,void*,SIZE_T,ULONG,ULONG);
static void     (WINAPI *pNotifyMemoryAlloc)(void*,SIZE_T,ULONG,ULONG,BOOL,NTSTATUS);
static void     (WINAPI *pNotifyMemoryFree)(void*,SIZE_T,ULONG,BOOL,NTSTATUS);
static void     (WINAPI *pNotifyMemoryProtect)(void*,SIZE_T,ULONG,BOOL,NTSTATUS);
static void     (WINAPI *pNotifyUnmapViewOfSection)(void*,BOOL,NTSTATUS);
static NTSTATUS (WINAPI *pProcessInit)(void);
static void     (WINAPI *pProcessTerm)(HANDLE,BOOL,NTSTATUS);
static void     (WINAPI *pResetToConsistentState)(EXCEPTION_RECORD*,CONTEXT*,ARM64_NT_CONTEXT*);
static NTSTATUS (WINAPI *pThreadInit)(void);
static void     (WINAPI *pThreadTerm)(HANDLE,LONG);
static void     (WINAPI *pUpdateProcessorInformation)(SYSTEM_CPU_INFORMATION*);

static BOOLEAN emulated_processor_features[PROCESSOR_FEATURE_MAX];
static BYTE KiUserExceptionDispatcher_orig[16]; /* to detect patching */
extern void KiUserExceptionDispatcher_thunk(void) asm("EXP+#KiUserExceptionDispatcher");

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

static inline BOOL enter_syscall_callback(void)
{
    if (get_arm64ec_cpu_area()->InSyscallCallback) return FALSE;
    get_arm64ec_cpu_area()->InSyscallCallback = 1;
    return TRUE;
}

static inline void leave_syscall_callback(void)
{
    get_arm64ec_cpu_area()->InSyscallCallback = 0;
}

/**********************************************************************
 *           create_cross_process_work_list
 */
static NTSTATUS create_cross_process_work_list( CHPEV2_PROCESS_INFO *info )
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
    status = NtMapViewOfSection( section, GetCurrentProcess(), (void **)&list, 0, 0, NULL,
                                 &map_size, ViewShare, MEM_TOP_DOWN, PAGE_READWRITE );
    if (status)
    {
        NtClose( section );
        return status;
    }

    end = (CROSS_PROCESS_WORK_ENTRY *)((char *)list + map_size);
    for (i = 0; list->entries + i + 1 <= end; i++)
        RtlWow64PushCrossProcessWorkOntoFreeList( &list->free_list, &list->entries[i] );

    info->SectionHandle = section;
    info->CrossProcessWorkList = list;
    return STATUS_SUCCESS;
}


/**********************************************************************
 *           send_cross_process_notification
 */
static BOOL send_cross_process_notification( HANDLE process, UINT id, const void *addr, SIZE_T size,
                                             int nb_args, ... )
{
    CROSS_PROCESS_WORK_LIST *list;
    CROSS_PROCESS_WORK_ENTRY *entry;
    void *unused;
    HANDLE section;
    va_list args;
    int i;

    RtlOpenCrossProcessEmulatorWorkConnection( process, &section, (void **)&list );
    if (!list) return FALSE;
    if ((entry = RtlWow64PopCrossProcessWorkFromFreeList( &list->free_list )))
    {
        entry->id = id;
        entry->addr = (ULONG_PTR)addr;
        entry->size = size;
        if (nb_args)
        {
            va_start( args, nb_args );
            for (i = 0; i < nb_args; i++) entry->args[i] = va_arg( args, int );
            va_end( args );
        }
        RtlWow64PushCrossProcessWorkOntoWorkList( &list->work_list, entry, &unused );
    }
    NtUnmapViewOfSection( GetCurrentProcess(), list );
    NtClose( section );
    return TRUE;
}


static void *arm64ec_redirect_ptr( HMODULE module, void *ptr, const IMAGE_ARM64EC_METADATA *metadata )
{
    const IMAGE_ARM64EC_REDIRECTION_ENTRY *map = get_rva( module, metadata->RedirectionMetadata );
    int min = 0, max = metadata->RedirectionMetadataCount - 1;
    ULONG_PTR rva = (ULONG_PTR)ptr - (ULONG_PTR)module;

    if (!ptr) return NULL;
    while (min <= max)
    {
        int pos = (min + max) / 2;
        if (map[pos].Source == rva) return get_rva( module, map[pos].Destination );
        if (map[pos].Source < rva) min = pos + 1;
        else max = pos - 1;
    }
    return ptr;
}

static void arm64x_check_call(void);

/*******************************************************************
 *         arm64ec_process_init
 */
NTSTATUS arm64ec_process_init( HMODULE module )
{
    NTSTATUS status = STATUS_SUCCESS;
    CHPEV2_PROCESS_INFO *info = (CHPEV2_PROCESS_INFO *)(RtlGetCurrentPeb() + 1);
    const IMAGE_ARM64EC_METADATA *metadata = arm64ec_get_module_metadata( module );

    __os_arm64x_dispatch_call_no_redirect = RtlFindExportedRoutineByName( module, "ExitToX64" );
    __os_arm64x_dispatch_fptr = RtlFindExportedRoutineByName( module, "DispatchJump" );
    __os_arm64x_dispatch_ret = RtlFindExportedRoutineByName( module, "RetToEntryThunk" );

#define GET_PTR(name) p ## name = arm64ec_redirect_ptr( module, \
                                      RtlFindExportedRoutineByName( module, #name ), metadata )
    GET_PTR( BTCpu64FlushInstructionCache );
    GET_PTR( BTCpu64IsProcessorFeaturePresent );
    GET_PTR( BTCpu64NotifyMemoryDirty );
    GET_PTR( BTCpu64NotifyReadFile );
    GET_PTR( BeginSimulation );
    GET_PTR( FlushInstructionCacheHeavy );
    GET_PTR( NotifyMapViewOfSection );
    GET_PTR( NotifyMemoryAlloc );
    GET_PTR( NotifyMemoryFree );
    GET_PTR( NotifyMemoryProtect );
    GET_PTR( NotifyUnmapViewOfSection );
    GET_PTR( ProcessInit );
    GET_PTR( ProcessTerm );
    GET_PTR( ResetToConsistentState );
    GET_PTR( ThreadInit );
    GET_PTR( ThreadTerm );
    GET_PTR( UpdateProcessorInformation );
#undef GET_PTR

    RtlGetCurrentPeb()->ChpeV2ProcessInfo = info;
    info->NativeMachineType = IMAGE_FILE_MACHINE_ARM64;
    info->EmulatedMachineType = IMAGE_FILE_MACHINE_AMD64;
    memcpy( KiUserExceptionDispatcher_orig, KiUserExceptionDispatcher_thunk, sizeof(KiUserExceptionDispatcher_orig) );

    enter_syscall_callback();
    if (pProcessInit) status = pProcessInit();
    if (!status)
    {
        for (unsigned int i = 0; i < PROCESSOR_FEATURE_MAX; i++)
            emulated_processor_features[i] = pBTCpu64IsProcessorFeaturePresent( i );
        status = create_cross_process_work_list( info );
    }
    if (!status && pThreadInit) status = pThreadInit();
    leave_syscall_callback();
    __os_arm64x_check_call = arm64x_check_call;
    __os_arm64x_check_icall = arm64x_check_call;
    __os_arm64x_check_icall_cfg = arm64x_check_call;
    return status;
}


/*******************************************************************
 *         arm64ec_thread_init
 */
NTSTATUS arm64ec_thread_init(void)
{
    NTSTATUS status = STATUS_SUCCESS;

    enter_syscall_callback();
    if (pThreadInit) status = pThreadInit();
    leave_syscall_callback();
    return status;
}


/**********************************************************************
 *           arm64ec_get_module_metadata
 */
IMAGE_ARM64EC_METADATA *arm64ec_get_module_metadata( HMODULE module )
{
    IMAGE_LOAD_CONFIG_DIRECTORY *cfg;
    ULONG size;

    if (!(cfg = RtlImageDirectoryEntryToData( module, TRUE,
                                              IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG, &size )))
        return NULL;

    size = min( size, cfg->Size );
    if (size <= offsetof( IMAGE_LOAD_CONFIG_DIRECTORY, CHPEMetadataPointer )) return NULL;
    return (IMAGE_ARM64EC_METADATA *)cfg->CHPEMetadataPointer;
}


static void update_hybrid_pointer( void *module, const IMAGE_SECTION_HEADER *sec, UINT rva, void *ptr )
{
    if (!rva) return;

    if (rva < sec->VirtualAddress || rva >= sec->VirtualAddress + sec->Misc.VirtualSize)
        ERR( "rva %x outside of section %s (%lx-%lx)\n", rva,
             sec->Name, sec->VirtualAddress, sec->VirtualAddress + sec->Misc.VirtualSize );
    else
        *(void **)get_rva( module, rva ) = ptr;
}

/*******************************************************************
 *         arm64ec_update_hybrid_metadata
 */
void arm64ec_update_hybrid_metadata( void *module, IMAGE_NT_HEADERS *nt,
                                     const IMAGE_ARM64EC_METADATA *metadata )
{
    DWORD i, protect_old;
    const IMAGE_SECTION_HEADER *sec = IMAGE_FIRST_SECTION( nt );

    /* assume that all pointers are in the same section */

    for (i = 0; i < nt->FileHeader.NumberOfSections; i++, sec++)
    {
        if ((sec->VirtualAddress <= metadata->__os_arm64x_dispatch_call) &&
            (sec->VirtualAddress + sec->Misc.VirtualSize > metadata->__os_arm64x_dispatch_call))
        {
            void *base = get_rva( module, sec->VirtualAddress );
            SIZE_T size = sec->Misc.VirtualSize;

            NtProtectVirtualMemory( NtCurrentProcess(), &base, &size, PAGE_READWRITE, &protect_old );

#define SET_FUNC(func,val) update_hybrid_pointer( module, sec, metadata->func, val )
            SET_FUNC( __os_arm64x_dispatch_call, arm64x_check_call );
            SET_FUNC( __os_arm64x_dispatch_call_no_redirect, __os_arm64x_dispatch_call_no_redirect );
            SET_FUNC( __os_arm64x_dispatch_fptr, __os_arm64x_dispatch_fptr );
            SET_FUNC( __os_arm64x_dispatch_icall, arm64x_check_call );
            SET_FUNC( __os_arm64x_dispatch_icall_cfg, arm64x_check_call );
            SET_FUNC( __os_arm64x_dispatch_ret, __os_arm64x_dispatch_ret );
            SET_FUNC( __os_arm64x_helper3, __os_arm64x_helper3 );
            SET_FUNC( __os_arm64x_helper4, __os_arm64x_helper4 );
            SET_FUNC( __os_arm64x_helper5, __os_arm64x_helper5 );
            SET_FUNC( __os_arm64x_helper6, __os_arm64x_helper6 );
            SET_FUNC( __os_arm64x_helper7, __os_arm64x_helper7 );
            SET_FUNC( __os_arm64x_helper8, __os_arm64x_helper8 );
            SET_FUNC( GetX64InformationFunctionPointer, __os_arm64x_get_x64_information );
            SET_FUNC( SetX64InformationFunctionPointer, __os_arm64x_set_x64_information );
#undef SET_FUNC

            NtProtectVirtualMemory( NtCurrentProcess(), &base, &size, protect_old, &protect_old );
            return;
        }
    }
    ERR( "module %p no section found for %lx\n", module, metadata->__os_arm64x_dispatch_call );
}


/*******************************************************************
 *         syscalls
 */
enum syscall_ids
{
#define SYSCALL_ENTRY(id,name,args) __id_##name = id,
ALL_SYSCALLS
#undef SYSCALL_ENTRY
    __nb_syscalls
};

#define DEFINE_SYSCALL_(ret,name,args) \
    ret __attribute__((naked, hybrid_patchable)) name args { __ASM_SYSCALL_FUNC( __id_##name, name ); }

#define DEFINE_SYSCALL(name,args) DEFINE_SYSCALL_(NTSTATUS,name,args)

#define DEFINE_WRAPPED_SYSCALL(name,args) \
    static NTSTATUS __attribute__((naked)) syscall_##name args { __ASM_SYSCALL_FUNC( __id_##name, syscall_##name ); }

#define SYSCALL_API __attribute__((hybrid_patchable))

DEFINE_SYSCALL(NtAcceptConnectPort, (HANDLE *handle, ULONG id, LPC_MESSAGE *msg, BOOLEAN accept, LPC_SECTION_WRITE *write, LPC_SECTION_READ *read))
DEFINE_SYSCALL(NtAccessCheck, (PSECURITY_DESCRIPTOR descr, HANDLE token, ACCESS_MASK access, GENERIC_MAPPING *mapping, PRIVILEGE_SET *privs, ULONG *retlen, ULONG *access_granted, NTSTATUS *access_status))
DEFINE_SYSCALL(NtAccessCheckAndAuditAlarm, (UNICODE_STRING *subsystem, HANDLE handle, UNICODE_STRING *typename, UNICODE_STRING *objectname, PSECURITY_DESCRIPTOR descr, ACCESS_MASK access, GENERIC_MAPPING *mapping, BOOLEAN creation, ACCESS_MASK *access_granted, NTSTATUS *access_status, BOOLEAN *onclose))
DEFINE_SYSCALL(NtAccessCheckByTypeAndAuditAlarm, (UNICODE_STRING *subsystem, HANDLE handle, UNICODE_STRING *typename, UNICODE_STRING *objectname, PSECURITY_DESCRIPTOR descr, PSID sid, ACCESS_MASK access, AUDIT_EVENT_TYPE audit_type, ULONG flags, OBJECT_TYPE_LIST *obj_list, ULONG list_len, GENERIC_MAPPING *mapping, BOOLEAN creation, ACCESS_MASK *access_granted, NTSTATUS *access_status, BOOLEAN *onclose))
DEFINE_SYSCALL(NtAddAtom, (const WCHAR *name, ULONG length, RTL_ATOM *atom))
DEFINE_SYSCALL(NtAdjustGroupsToken, (HANDLE token, BOOLEAN reset, TOKEN_GROUPS *groups, ULONG length, TOKEN_GROUPS *prev, ULONG *retlen))
DEFINE_SYSCALL(NtAdjustPrivilegesToken, (HANDLE token, BOOLEAN disable, TOKEN_PRIVILEGES *privs, DWORD length, TOKEN_PRIVILEGES *prev, DWORD *retlen))
DEFINE_SYSCALL(NtAlertMultipleThreadByThreadId, (HANDLE *tids, ULONG count, void *unk1, void *unk2))
DEFINE_SYSCALL(NtAlertResumeThread, (HANDLE handle, ULONG *count))
DEFINE_SYSCALL(NtAlertThread, (HANDLE handle))
DEFINE_SYSCALL(NtAlertThreadByThreadId, (HANDLE tid))
DEFINE_SYSCALL(NtAllocateLocallyUniqueId, (LUID *luid))
DEFINE_SYSCALL(NtAllocateReserveObject, (HANDLE *handle, const OBJECT_ATTRIBUTES *attr, MEMORY_RESERVE_OBJECT_TYPE type))
DEFINE_SYSCALL(NtAllocateUuids, (ULARGE_INTEGER *time, ULONG *delta, ULONG *sequence, UCHAR *seed))
DEFINE_WRAPPED_SYSCALL(NtAllocateVirtualMemory, (HANDLE process, PVOID *ret, ULONG_PTR zero_bits, SIZE_T *size_ptr, ULONG type, ULONG protect))
DEFINE_WRAPPED_SYSCALL(NtAllocateVirtualMemoryEx, (HANDLE process, PVOID *ret, SIZE_T *size_ptr, ULONG type, ULONG protect, MEM_EXTENDED_PARAMETER *parameters, ULONG count))
DEFINE_SYSCALL(NtApphelpCacheControl, (ULONG class, void *context))
DEFINE_SYSCALL(NtAreMappedFilesTheSame, (PVOID addr1, PVOID addr2))
DEFINE_SYSCALL(NtAssignProcessToJobObject, (HANDLE job, HANDLE process))
DEFINE_SYSCALL(NtCallbackReturn, (void *ret_ptr, ULONG ret_len, NTSTATUS status))
DEFINE_SYSCALL(NtCancelIoFile, (HANDLE handle, IO_STATUS_BLOCK *io_status))
DEFINE_SYSCALL(NtCancelIoFileEx, (HANDLE handle, IO_STATUS_BLOCK *io, IO_STATUS_BLOCK *io_status))
DEFINE_SYSCALL(NtCancelSynchronousIoFile, (HANDLE handle, IO_STATUS_BLOCK *io, IO_STATUS_BLOCK *io_status))
DEFINE_SYSCALL(NtCancelTimer, (HANDLE handle, BOOLEAN *state))
DEFINE_SYSCALL(NtClearEvent, (HANDLE handle))
DEFINE_SYSCALL(NtClose, (HANDLE handle))
DEFINE_SYSCALL(NtCloseObjectAuditAlarm, (UNICODE_STRING *subsystem, HANDLE handle, BOOLEAN onclose))
DEFINE_SYSCALL(NtCommitTransaction, (HANDLE transaction, BOOLEAN wait))
DEFINE_SYSCALL(NtCompareObjects, (HANDLE first, HANDLE second))
DEFINE_SYSCALL(NtCompareTokens, (HANDLE first, HANDLE second, BOOLEAN *equal))
DEFINE_SYSCALL(NtCompleteConnectPort, (HANDLE handle))
DEFINE_SYSCALL(NtConnectPort, (HANDLE *handle, UNICODE_STRING *name, SECURITY_QUALITY_OF_SERVICE *qos, LPC_SECTION_WRITE *write, LPC_SECTION_READ *read, ULONG *max_len, void *info, ULONG *info_len))
DEFINE_WRAPPED_SYSCALL(NtContinue, (ARM64_NT_CONTEXT *context, BOOLEAN alertable))
DEFINE_WRAPPED_SYSCALL(NtContinueEx, (ARM64_NT_CONTEXT *context, KCONTINUE_ARGUMENT *args))
DEFINE_SYSCALL(NtConvertBetweenAuxiliaryCounterAndPerformanceCounter, (ULONG flag, ULONGLONG *from, ULONGLONG *to, ULONGLONG *error))
DEFINE_SYSCALL(NtCreateDebugObject, (HANDLE *handle, ACCESS_MASK access, OBJECT_ATTRIBUTES *attr, ULONG flags))
DEFINE_SYSCALL(NtCreateDirectoryObject, (HANDLE *handle, ACCESS_MASK access, OBJECT_ATTRIBUTES *attr))
DEFINE_SYSCALL(NtCreateEvent, (HANDLE *handle, ACCESS_MASK access, const OBJECT_ATTRIBUTES *attr, EVENT_TYPE type, BOOLEAN state))
DEFINE_SYSCALL(NtCreateFile, (HANDLE *handle, ACCESS_MASK access, OBJECT_ATTRIBUTES *attr, IO_STATUS_BLOCK *io, LARGE_INTEGER *alloc_size, ULONG attributes, ULONG sharing, ULONG disposition, ULONG options, void *ea_buffer, ULONG ea_length))
DEFINE_SYSCALL(NtCreateIoCompletion, (HANDLE *handle, ACCESS_MASK access, OBJECT_ATTRIBUTES *attr, ULONG threads))
DEFINE_SYSCALL(NtCreateJobObject, (HANDLE *handle, ACCESS_MASK access, const OBJECT_ATTRIBUTES *attr))
DEFINE_SYSCALL(NtCreateKey, (HANDLE *key, ACCESS_MASK access, const OBJECT_ATTRIBUTES *attr, ULONG index, const UNICODE_STRING *class, ULONG options, ULONG *dispos))
DEFINE_SYSCALL(NtCreateKeyTransacted, (HANDLE *key, ACCESS_MASK access, const OBJECT_ATTRIBUTES *attr, ULONG index, const UNICODE_STRING *class, ULONG options, HANDLE transacted, ULONG *dispos))
DEFINE_SYSCALL(NtCreateKeyedEvent, (HANDLE *handle, ACCESS_MASK access, const OBJECT_ATTRIBUTES *attr, ULONG flags))
DEFINE_SYSCALL(NtCreateLowBoxToken, (HANDLE *token_handle, HANDLE token, ACCESS_MASK access, OBJECT_ATTRIBUTES *attr, SID *sid, ULONG count, SID_AND_ATTRIBUTES *capabilities, ULONG handle_count, HANDLE *handle))
DEFINE_SYSCALL(NtCreateMailslotFile, (HANDLE *handle, ULONG access, OBJECT_ATTRIBUTES *attr, IO_STATUS_BLOCK *io, ULONG options, ULONG quota, ULONG msg_size, LARGE_INTEGER *timeout))
DEFINE_SYSCALL(NtCreateMutant, (HANDLE *handle, ACCESS_MASK access, const OBJECT_ATTRIBUTES *attr, BOOLEAN owned))
DEFINE_SYSCALL(NtCreateNamedPipeFile, (HANDLE *handle, ULONG access, OBJECT_ATTRIBUTES *attr, IO_STATUS_BLOCK *io, ULONG sharing, ULONG dispo, ULONG options, ULONG pipe_type, ULONG read_mode, ULONG completion_mode, ULONG max_inst, ULONG inbound_quota, ULONG outbound_quota, LARGE_INTEGER *timeout))
DEFINE_SYSCALL(NtCreatePagingFile, (UNICODE_STRING *name, LARGE_INTEGER *min_size, LARGE_INTEGER *max_size, LARGE_INTEGER *actual_size))
DEFINE_SYSCALL(NtCreatePort, (HANDLE *handle, OBJECT_ATTRIBUTES *attr, ULONG info_len, ULONG data_len, ULONG *reserved))
DEFINE_SYSCALL(NtCreateProcessEx, (HANDLE *handle, ACCESS_MASK access, OBJECT_ATTRIBUTES *attr, HANDLE parent, ULONG flags, HANDLE section, HANDLE debug, HANDLE token, ULONG reserved))
DEFINE_SYSCALL(NtCreateSection, (HANDLE *handle, ACCESS_MASK access, const OBJECT_ATTRIBUTES *attr, const LARGE_INTEGER *size, ULONG protect, ULONG sec_flags, HANDLE file))
DEFINE_SYSCALL(NtCreateSectionEx, (HANDLE *handle, ACCESS_MASK access, const OBJECT_ATTRIBUTES *attr, const LARGE_INTEGER *size, ULONG protect, ULONG sec_flags, HANDLE file, MEM_EXTENDED_PARAMETER *parameters, ULONG count))
DEFINE_SYSCALL(NtCreateSemaphore, (HANDLE *handle, ACCESS_MASK access, const OBJECT_ATTRIBUTES *attr, LONG initial, LONG max))
DEFINE_SYSCALL(NtCreateSymbolicLinkObject, (HANDLE *handle, ACCESS_MASK access, OBJECT_ATTRIBUTES *attr, UNICODE_STRING *target))
DEFINE_SYSCALL(NtCreateThread, (HANDLE *handle, ACCESS_MASK access, OBJECT_ATTRIBUTES *attr, HANDLE process, CLIENT_ID *id, CONTEXT *ctx, INITIAL_TEB *teb, BOOLEAN suspended))
DEFINE_SYSCALL(NtCreateThreadEx, (HANDLE *handle, ACCESS_MASK access, OBJECT_ATTRIBUTES *attr, HANDLE process, PRTL_THREAD_START_ROUTINE start, void *param, ULONG flags, ULONG_PTR zero_bits, SIZE_T stack_commit, SIZE_T stack_reserve, PS_ATTRIBUTE_LIST *attr_list))
DEFINE_SYSCALL(NtCreateTimer, (HANDLE *handle, ACCESS_MASK access, const OBJECT_ATTRIBUTES *attr, TIMER_TYPE type))
DEFINE_SYSCALL(NtCreateToken, (HANDLE *handle, ACCESS_MASK access, OBJECT_ATTRIBUTES *attr, TOKEN_TYPE type, LUID *token_id, LARGE_INTEGER *expire, TOKEN_USER *user, TOKEN_GROUPS *groups, TOKEN_PRIVILEGES *privs, TOKEN_OWNER *owner, TOKEN_PRIMARY_GROUP *group, TOKEN_DEFAULT_DACL *dacl, TOKEN_SOURCE *source))
DEFINE_SYSCALL(NtCreateTransaction, (HANDLE *handle, ACCESS_MASK mask, OBJECT_ATTRIBUTES *obj_attr, GUID *guid, HANDLE tm, ULONG options, ULONG isol_level, ULONG isol_flags, PLARGE_INTEGER timeout, UNICODE_STRING *description))
DEFINE_SYSCALL(NtCreateUserProcess, (HANDLE *process_handle_ptr, HANDLE *thread_handle_ptr, ACCESS_MASK process_access, ACCESS_MASK thread_access, OBJECT_ATTRIBUTES *process_attr, OBJECT_ATTRIBUTES *thread_attr, ULONG process_flags, ULONG thread_flags, RTL_USER_PROCESS_PARAMETERS *params, PS_CREATE_INFO *info, PS_ATTRIBUTE_LIST *ps_attr))
DEFINE_SYSCALL(NtDebugActiveProcess, (HANDLE process, HANDLE debug))
DEFINE_SYSCALL(NtDebugContinue, (HANDLE handle, CLIENT_ID *client, NTSTATUS status))
DEFINE_SYSCALL(NtDelayExecution, (BOOLEAN alertable, const LARGE_INTEGER *timeout))
DEFINE_SYSCALL(NtDeleteAtom, (RTL_ATOM atom))
DEFINE_SYSCALL(NtDeleteFile, (OBJECT_ATTRIBUTES *attr))
DEFINE_SYSCALL(NtDeleteKey, (HANDLE key))
DEFINE_SYSCALL(NtDeleteValueKey, (HANDLE key, const UNICODE_STRING *name))
DEFINE_SYSCALL(NtDeviceIoControlFile, (HANDLE handle, HANDLE event, PIO_APC_ROUTINE apc, void *apc_context, IO_STATUS_BLOCK *io, ULONG code, void *in_buffer, ULONG in_size, void *out_buffer, ULONG out_size))
DEFINE_SYSCALL(NtDisplayString, (UNICODE_STRING *string))
DEFINE_SYSCALL(NtDuplicateObject, (HANDLE source_process, HANDLE source, HANDLE dest_process, HANDLE *dest, ACCESS_MASK access, ULONG attributes, ULONG options))
DEFINE_SYSCALL(NtDuplicateToken, (HANDLE token, ACCESS_MASK access, OBJECT_ATTRIBUTES *attr, BOOLEAN effective_only, TOKEN_TYPE type, HANDLE *handle))
DEFINE_SYSCALL(NtEnumerateKey, (HANDLE handle, ULONG index, KEY_INFORMATION_CLASS info_class, void *info, DWORD length, DWORD *result_len))
DEFINE_SYSCALL(NtEnumerateValueKey, (HANDLE handle, ULONG index, KEY_VALUE_INFORMATION_CLASS info_class, void *info, DWORD length, DWORD *result_len))
DEFINE_SYSCALL(NtFilterToken, (HANDLE token, ULONG flags, TOKEN_GROUPS *disable_sids, TOKEN_PRIVILEGES *privileges, TOKEN_GROUPS *restrict_sids, HANDLE *new_token))
DEFINE_SYSCALL(NtFindAtom, (const WCHAR *name, ULONG length, RTL_ATOM *atom))
DEFINE_SYSCALL(NtFlushBuffersFile, (HANDLE handle, IO_STATUS_BLOCK *io))
DEFINE_SYSCALL(NtFlushBuffersFileEx, (HANDLE handle, ULONG flags, void *params, ULONG size, IO_STATUS_BLOCK *io))
DEFINE_WRAPPED_SYSCALL(NtFlushInstructionCache, (HANDLE handle, const void *addr, SIZE_T size))
DEFINE_SYSCALL(NtFlushKey, (HANDLE key))
DEFINE_SYSCALL(NtFlushProcessWriteBuffers, (void))
DEFINE_SYSCALL(NtFlushVirtualMemory, (HANDLE process, LPCVOID *addr_ptr, SIZE_T *size_ptr, ULONG unknown))
DEFINE_WRAPPED_SYSCALL(NtFreeVirtualMemory, (HANDLE process, PVOID *addr_ptr, SIZE_T *size_ptr, ULONG type))
DEFINE_SYSCALL(NtFsControlFile, (HANDLE handle, HANDLE event, PIO_APC_ROUTINE apc, void *apc_context, IO_STATUS_BLOCK *io, ULONG code, void *in_buffer, ULONG in_size, void *out_buffer, ULONG out_size))
DEFINE_WRAPPED_SYSCALL(NtGetContextThread, (HANDLE handle, ARM64_NT_CONTEXT *context))
DEFINE_SYSCALL_(ULONG, NtGetCurrentProcessorNumber, (void))
DEFINE_SYSCALL(NtGetNextProcess, (HANDLE process, ACCESS_MASK access, ULONG attributes, ULONG flags, HANDLE *handle))
DEFINE_SYSCALL(NtGetNextThread, (HANDLE process, HANDLE thread, ACCESS_MASK access, ULONG attributes, ULONG flags, HANDLE *handle))
DEFINE_SYSCALL(NtGetNlsSectionPtr, (ULONG type, ULONG id, void *unknown, void **ptr, SIZE_T *size))
DEFINE_SYSCALL(NtGetWriteWatch, (HANDLE process, ULONG flags, PVOID base, SIZE_T size, PVOID *addresses, ULONG_PTR *count, ULONG *granularity))
DEFINE_SYSCALL(NtImpersonateAnonymousToken, (HANDLE thread))
DEFINE_SYSCALL(NtImpersonateClientOfPort, (HANDLE handle, LPC_MESSAGE *request))
DEFINE_SYSCALL(NtInitializeNlsFiles, (void **ptr, LCID *lcid, LARGE_INTEGER *size))
DEFINE_SYSCALL(NtInitiatePowerAction, (POWER_ACTION action, SYSTEM_POWER_STATE state, ULONG flags, BOOLEAN async))
DEFINE_SYSCALL(NtIsProcessInJob, (HANDLE process, HANDLE job))
DEFINE_SYSCALL(NtListenPort, (HANDLE handle, LPC_MESSAGE *msg))
DEFINE_SYSCALL(NtLoadDriver, (const UNICODE_STRING *name))
DEFINE_SYSCALL(NtLoadKey, (const OBJECT_ATTRIBUTES *attr, OBJECT_ATTRIBUTES *file))
DEFINE_SYSCALL(NtLoadKey2, (const OBJECT_ATTRIBUTES *attr, OBJECT_ATTRIBUTES *file, ULONG flags))
DEFINE_SYSCALL(NtLoadKeyEx, (const OBJECT_ATTRIBUTES *attr, OBJECT_ATTRIBUTES *file, ULONG flags, HANDLE trustkey, HANDLE event, ACCESS_MASK access, HANDLE *roothandle, IO_STATUS_BLOCK *iostatus))
DEFINE_SYSCALL(NtLockFile, (HANDLE file, HANDLE event, PIO_APC_ROUTINE apc, void* apc_user, IO_STATUS_BLOCK *io_status, LARGE_INTEGER *offset, LARGE_INTEGER *count, ULONG *key, BOOLEAN dont_wait, BOOLEAN exclusive))
DEFINE_SYSCALL(NtLockVirtualMemory, (HANDLE process, PVOID *addr, SIZE_T *size, ULONG unknown))
DEFINE_SYSCALL(NtMakePermanentObject, (HANDLE handle))
DEFINE_SYSCALL(NtMakeTemporaryObject, (HANDLE handle))
DEFINE_SYSCALL(NtMapUserPhysicalPagesScatter, (void **addr, SIZE_T count, ULONG_PTR *pages))
DEFINE_WRAPPED_SYSCALL(NtMapViewOfSection, (HANDLE handle, HANDLE process, PVOID *addr_ptr, ULONG_PTR zero_bits, SIZE_T commit_size, const LARGE_INTEGER *offset_ptr, SIZE_T *size_ptr, SECTION_INHERIT inherit, ULONG alloc_type, ULONG protect))
DEFINE_WRAPPED_SYSCALL(NtMapViewOfSectionEx, (HANDLE handle, HANDLE process, PVOID *addr_ptr, const LARGE_INTEGER *offset_ptr, SIZE_T *size_ptr, ULONG alloc_type, ULONG protect, MEM_EXTENDED_PARAMETER *parameters, ULONG count))
DEFINE_SYSCALL(NtNotifyChangeDirectoryFile, (HANDLE handle, HANDLE event, PIO_APC_ROUTINE apc, void *apc_context, IO_STATUS_BLOCK *iosb, void *buffer, ULONG buffer_size, ULONG filter, BOOLEAN subtree))
DEFINE_SYSCALL(NtNotifyChangeKey, (HANDLE key, HANDLE event, PIO_APC_ROUTINE apc, void *apc_context, IO_STATUS_BLOCK *io, ULONG filter, BOOLEAN subtree, void *buffer, ULONG length, BOOLEAN async))
DEFINE_SYSCALL(NtNotifyChangeMultipleKeys, (HANDLE key, ULONG count, OBJECT_ATTRIBUTES *attr, HANDLE event, PIO_APC_ROUTINE apc, void *apc_context, IO_STATUS_BLOCK *io, ULONG filter, BOOLEAN subtree, void *buffer, ULONG length, BOOLEAN async))
DEFINE_SYSCALL(NtOpenDirectoryObject, (HANDLE *handle, ACCESS_MASK access, const OBJECT_ATTRIBUTES *attr))
DEFINE_SYSCALL(NtOpenEvent, (HANDLE *handle, ACCESS_MASK access, const OBJECT_ATTRIBUTES *attr))
DEFINE_SYSCALL(NtOpenFile, (HANDLE *handle, ACCESS_MASK access, OBJECT_ATTRIBUTES *attr, IO_STATUS_BLOCK *io, ULONG sharing, ULONG options))
DEFINE_SYSCALL(NtOpenIoCompletion, (HANDLE *handle, ACCESS_MASK access, const OBJECT_ATTRIBUTES *attr))
DEFINE_SYSCALL(NtOpenJobObject, (HANDLE *handle, ACCESS_MASK access, const OBJECT_ATTRIBUTES *attr))
DEFINE_SYSCALL(NtOpenKey, (HANDLE *key, ACCESS_MASK access, const OBJECT_ATTRIBUTES *attr))
DEFINE_SYSCALL(NtOpenKeyEx, (HANDLE *key, ACCESS_MASK access, const OBJECT_ATTRIBUTES *attr, ULONG options))
DEFINE_SYSCALL(NtOpenKeyTransacted, (HANDLE *key, ACCESS_MASK access, const OBJECT_ATTRIBUTES *attr, HANDLE transaction))
DEFINE_SYSCALL(NtOpenKeyTransactedEx, (HANDLE *key, ACCESS_MASK access, const OBJECT_ATTRIBUTES *attr, ULONG options, HANDLE transaction))
DEFINE_SYSCALL(NtOpenKeyedEvent, (HANDLE *handle, ACCESS_MASK access, const OBJECT_ATTRIBUTES *attr))
DEFINE_SYSCALL(NtOpenMutant, (HANDLE *handle, ACCESS_MASK access, const OBJECT_ATTRIBUTES *attr))
DEFINE_SYSCALL(NtOpenProcess, (HANDLE *handle, ACCESS_MASK access, const OBJECT_ATTRIBUTES *attr, const CLIENT_ID *id))
DEFINE_SYSCALL(NtOpenProcessToken, (HANDLE process, DWORD access, HANDLE *handle))
DEFINE_SYSCALL(NtOpenProcessTokenEx, (HANDLE process, DWORD access, DWORD attributes, HANDLE *handle))
DEFINE_SYSCALL(NtOpenSection, (HANDLE *handle, ACCESS_MASK access, const OBJECT_ATTRIBUTES *attr))
DEFINE_SYSCALL(NtOpenSemaphore, (HANDLE *handle, ACCESS_MASK access, const OBJECT_ATTRIBUTES *attr))
DEFINE_SYSCALL(NtOpenSymbolicLinkObject, (HANDLE *handle, ACCESS_MASK access, const OBJECT_ATTRIBUTES *attr))
DEFINE_SYSCALL(NtOpenThread, (HANDLE *handle, ACCESS_MASK access, const OBJECT_ATTRIBUTES *attr, const CLIENT_ID *id))
DEFINE_SYSCALL(NtOpenThreadToken, (HANDLE thread, DWORD access, BOOLEAN self, HANDLE *handle))
DEFINE_SYSCALL(NtOpenThreadTokenEx, (HANDLE thread, DWORD access, BOOLEAN self, DWORD attributes, HANDLE *handle))
DEFINE_SYSCALL(NtOpenTimer, (HANDLE *handle, ACCESS_MASK access, const OBJECT_ATTRIBUTES *attr))
DEFINE_SYSCALL(NtPowerInformation, (POWER_INFORMATION_LEVEL level, void *input, ULONG in_size, void *output, ULONG out_size))
DEFINE_SYSCALL(NtPrivilegeCheck, (HANDLE token, PRIVILEGE_SET *privs, BOOLEAN *res))
DEFINE_WRAPPED_SYSCALL(NtProtectVirtualMemory, (HANDLE process, PVOID *addr_ptr, SIZE_T *size_ptr, ULONG new_prot, ULONG *old_prot))
DEFINE_SYSCALL(NtPulseEvent, (HANDLE handle, LONG *prev_state))
DEFINE_SYSCALL(NtQueryAttributesFile, (const OBJECT_ATTRIBUTES *attr, FILE_BASIC_INFORMATION *info))
DEFINE_SYSCALL(NtQueryDefaultLocale, (BOOLEAN user, LCID *lcid))
DEFINE_SYSCALL(NtQueryDefaultUILanguage, (LANGID *lang))
DEFINE_SYSCALL(NtQueryDirectoryFile, (HANDLE handle, HANDLE event, PIO_APC_ROUTINE apc_routine, void *apc_context, IO_STATUS_BLOCK *io, void *buffer, ULONG length, FILE_INFORMATION_CLASS info_class, BOOLEAN single_entry, UNICODE_STRING *mask, BOOLEAN restart_scan))
DEFINE_SYSCALL(NtQueryDirectoryObject, (HANDLE handle, DIRECTORY_BASIC_INFORMATION *buffer, ULONG size, BOOLEAN single_entry, BOOLEAN restart, ULONG *context, ULONG *ret_size))
DEFINE_SYSCALL(NtQueryEaFile, (HANDLE handle, IO_STATUS_BLOCK *io, void *buffer, ULONG length, BOOLEAN single_entry, void *list, ULONG list_len, ULONG *index, BOOLEAN restart))
DEFINE_SYSCALL(NtQueryEvent, (HANDLE handle, EVENT_INFORMATION_CLASS class, void *info, ULONG len, ULONG *ret_len))
DEFINE_SYSCALL(NtQueryFullAttributesFile, (const OBJECT_ATTRIBUTES *attr, FILE_NETWORK_OPEN_INFORMATION *info))
DEFINE_SYSCALL(NtQueryInformationAtom, (RTL_ATOM atom, ATOM_INFORMATION_CLASS class, void *ptr, ULONG size, ULONG *retsize))
DEFINE_SYSCALL(NtQueryInformationFile, (HANDLE handle, IO_STATUS_BLOCK *io, void *ptr, ULONG len, FILE_INFORMATION_CLASS class))
DEFINE_SYSCALL(NtQueryInformationJobObject, (HANDLE handle, JOBOBJECTINFOCLASS class, void *info, ULONG len, ULONG *ret_len))
DEFINE_SYSCALL(NtQueryInformationProcess, (HANDLE handle, PROCESSINFOCLASS class, void *info, ULONG size, ULONG *ret_len))
DEFINE_SYSCALL(NtQueryInformationThread, (HANDLE handle, THREADINFOCLASS class, void *data, ULONG length, ULONG *ret_len))
DEFINE_SYSCALL(NtQueryInformationToken, (HANDLE token, TOKEN_INFORMATION_CLASS class, void *info, ULONG length, ULONG *retlen))
DEFINE_SYSCALL(NtQueryInstallUILanguage, (LANGID *lang))
DEFINE_SYSCALL(NtQueryIoCompletion, (HANDLE handle, IO_COMPLETION_INFORMATION_CLASS class, void *buffer, ULONG len, ULONG *ret_len))
DEFINE_SYSCALL(NtQueryKey, (HANDLE handle, KEY_INFORMATION_CLASS info_class, void *info, DWORD length, DWORD *result_len))
DEFINE_SYSCALL(NtQueryLicenseValue, (const UNICODE_STRING *name, ULONG *type, void *data, ULONG length, ULONG *retlen))
DEFINE_SYSCALL(NtQueryMultipleValueKey, (HANDLE key, KEY_MULTIPLE_VALUE_INFORMATION *info, ULONG count, void *buffer, ULONG length, ULONG *retlen))
DEFINE_SYSCALL(NtQueryMutant, (HANDLE handle, MUTANT_INFORMATION_CLASS class, void *info, ULONG len, ULONG *ret_len))
DEFINE_SYSCALL(NtQueryObject, (HANDLE handle, OBJECT_INFORMATION_CLASS info_class, void *ptr, ULONG len, ULONG *used_len))
DEFINE_SYSCALL(NtQueryPerformanceCounter, (LARGE_INTEGER *counter, LARGE_INTEGER *frequency))
DEFINE_SYSCALL(NtQuerySection, (HANDLE handle, SECTION_INFORMATION_CLASS class, void *ptr, SIZE_T size, SIZE_T *ret_size))
DEFINE_SYSCALL(NtQuerySecurityObject, (HANDLE handle, SECURITY_INFORMATION info, PSECURITY_DESCRIPTOR descr, ULONG length, ULONG *retlen))
DEFINE_SYSCALL(NtQuerySemaphore, (HANDLE handle, SEMAPHORE_INFORMATION_CLASS class, void *info, ULONG len, ULONG *ret_len))
DEFINE_SYSCALL(NtQuerySymbolicLinkObject, (HANDLE handle, UNICODE_STRING *target, ULONG *length))
DEFINE_SYSCALL(NtQuerySystemEnvironmentValue, (UNICODE_STRING *name, WCHAR *buffer, ULONG length, ULONG *retlen))
DEFINE_SYSCALL(NtQuerySystemEnvironmentValueEx, (UNICODE_STRING *name, GUID *vendor, void *buffer, ULONG *retlen, ULONG *attrib))
DEFINE_WRAPPED_SYSCALL(NtQuerySystemInformation, (SYSTEM_INFORMATION_CLASS class, void *info, ULONG size, ULONG *ret_size))
DEFINE_SYSCALL(NtQuerySystemInformationEx, (SYSTEM_INFORMATION_CLASS class, void *query, ULONG query_len, void *info, ULONG size, ULONG *ret_size))
DEFINE_SYSCALL(NtQuerySystemTime, (LARGE_INTEGER *time))
DEFINE_SYSCALL(NtQueryTimer, (HANDLE handle, TIMER_INFORMATION_CLASS class, void *info, ULONG len, ULONG *ret_len))
DEFINE_SYSCALL(NtQueryTimerResolution, (ULONG *min_res, ULONG *max_res, ULONG *current_res))
DEFINE_SYSCALL(NtQueryValueKey, (HANDLE handle, const UNICODE_STRING *name, KEY_VALUE_INFORMATION_CLASS info_class, void *info, DWORD length, DWORD *result_len))
DEFINE_SYSCALL(NtQueryVirtualMemory, (HANDLE process, LPCVOID addr, MEMORY_INFORMATION_CLASS info_class, PVOID buffer, SIZE_T len, SIZE_T *res_len))
DEFINE_SYSCALL(NtQueryVolumeInformationFile, (HANDLE handle, IO_STATUS_BLOCK *io, void *buffer, ULONG length, FS_INFORMATION_CLASS info_class))
DEFINE_SYSCALL(NtQueueApcThread, (HANDLE handle, PNTAPCFUNC func, ULONG_PTR arg1, ULONG_PTR arg2, ULONG_PTR arg3))
DEFINE_SYSCALL(NtQueueApcThreadEx, (HANDLE handle, HANDLE reserve_handle, PNTAPCFUNC func, ULONG_PTR arg1, ULONG_PTR arg2, ULONG_PTR arg3))
DEFINE_SYSCALL(NtQueueApcThreadEx2, (HANDLE handle, HANDLE reserve_handle, ULONG flags, PNTAPCFUNC func, ULONG_PTR arg1, ULONG_PTR arg2, ULONG_PTR arg3))
DEFINE_WRAPPED_SYSCALL(NtRaiseException, (EXCEPTION_RECORD *rec, ARM64_NT_CONTEXT *context, BOOL first_chance))
DEFINE_SYSCALL(NtRaiseHardError, (NTSTATUS status, ULONG count, ULONG params_mask, void **params, HARDERROR_RESPONSE_OPTION option, HARDERROR_RESPONSE *response))
DEFINE_WRAPPED_SYSCALL(NtReadFile, (HANDLE handle, HANDLE event, PIO_APC_ROUTINE apc, void *apc_user, IO_STATUS_BLOCK *io, void *buffer, ULONG length, LARGE_INTEGER *offset, ULONG *key))
DEFINE_SYSCALL(NtReadFileScatter, (HANDLE file, HANDLE event, PIO_APC_ROUTINE apc, void *apc_user, IO_STATUS_BLOCK *io, FILE_SEGMENT_ELEMENT *segments, ULONG length, LARGE_INTEGER *offset, ULONG *key))
DEFINE_SYSCALL(NtReadRequestData, (HANDLE handle, LPC_MESSAGE *request, ULONG id, void *buffer, ULONG len, ULONG *retlen))
DEFINE_SYSCALL(NtReadVirtualMemory, (HANDLE process, const void *addr, void *buffer, SIZE_T size, SIZE_T *bytes_read))
DEFINE_SYSCALL(NtRegisterThreadTerminatePort, (HANDLE handle))
DEFINE_SYSCALL(NtReleaseKeyedEvent, (HANDLE handle, const void *key, BOOLEAN alertable, const LARGE_INTEGER *timeout))
DEFINE_SYSCALL(NtReleaseMutant, (HANDLE handle, LONG *prev_count))
DEFINE_SYSCALL(NtReleaseSemaphore, (HANDLE handle, ULONG count, ULONG *previous))
DEFINE_SYSCALL(NtRemoveIoCompletion, (HANDLE handle, ULONG_PTR *key, ULONG_PTR *value, IO_STATUS_BLOCK *io, LARGE_INTEGER *timeout))
DEFINE_SYSCALL(NtRemoveIoCompletionEx, (HANDLE handle, FILE_IO_COMPLETION_INFORMATION *info, ULONG count, ULONG *written, LARGE_INTEGER *timeout, BOOLEAN alertable))
DEFINE_SYSCALL(NtRemoveProcessDebug, (HANDLE process, HANDLE debug))
DEFINE_SYSCALL(NtRenameKey, (HANDLE key, UNICODE_STRING *name))
DEFINE_SYSCALL(NtReplaceKey, (OBJECT_ATTRIBUTES *attr, HANDLE key, OBJECT_ATTRIBUTES *replace))
DEFINE_SYSCALL(NtReplyPort, (HANDLE handle, LPC_MESSAGE *reply))
DEFINE_SYSCALL(NtReplyWaitReceivePort, (HANDLE handle, ULONG *id, LPC_MESSAGE *reply, LPC_MESSAGE *msg))
DEFINE_SYSCALL(NtReplyWaitReceivePortEx, (HANDLE handle, ULONG *id, LPC_MESSAGE *reply, LPC_MESSAGE *msg, LARGE_INTEGER *timeout))
DEFINE_SYSCALL(NtRequestWaitReplyPort, (HANDLE handle, LPC_MESSAGE *msg_in, LPC_MESSAGE *msg_out))
DEFINE_SYSCALL(NtResetEvent, (HANDLE handle, LONG *prev_state))
DEFINE_SYSCALL(NtResetWriteWatch, (HANDLE process, PVOID base, SIZE_T size))
DEFINE_SYSCALL(NtRestoreKey, (HANDLE key, HANDLE file, ULONG flags))
DEFINE_SYSCALL(NtResumeProcess, (HANDLE handle))
DEFINE_SYSCALL(NtResumeThread, (HANDLE handle, ULONG *count))
DEFINE_SYSCALL(NtRollbackTransaction, (HANDLE transaction, BOOLEAN wait))
DEFINE_SYSCALL(NtSaveKey, (HANDLE key, HANDLE file))
DEFINE_SYSCALL(NtSecureConnectPort, (HANDLE *handle, UNICODE_STRING *name, SECURITY_QUALITY_OF_SERVICE *qos, LPC_SECTION_WRITE *write, PSID sid, LPC_SECTION_READ *read, ULONG *max_len, void *info, ULONG *info_len))
DEFINE_WRAPPED_SYSCALL(NtSetContextThread, (HANDLE handle, const ARM64_NT_CONTEXT *context))
DEFINE_SYSCALL(NtSetDebugFilterState, (ULONG component_id, ULONG level, BOOLEAN state))
DEFINE_SYSCALL(NtSetDefaultLocale, (BOOLEAN user, LCID lcid))
DEFINE_SYSCALL(NtSetDefaultUILanguage, (LANGID lang))
DEFINE_SYSCALL(NtSetEaFile, (HANDLE handle, IO_STATUS_BLOCK *io, void *buffer, ULONG length))
DEFINE_SYSCALL(NtSetEvent, (HANDLE handle, LONG *prev_state))
DEFINE_SYSCALL(NtSetEventBoostPriority, (HANDLE handle))
DEFINE_SYSCALL(NtSetInformationDebugObject, (HANDLE handle, DEBUGOBJECTINFOCLASS class, void *info, ULONG len, ULONG *ret_len))
DEFINE_SYSCALL(NtSetInformationFile, (HANDLE handle, IO_STATUS_BLOCK *io, void *ptr, ULONG len, FILE_INFORMATION_CLASS class))
DEFINE_SYSCALL(NtSetInformationJobObject, (HANDLE handle, JOBOBJECTINFOCLASS class, void *info, ULONG len))
DEFINE_SYSCALL(NtSetInformationKey, (HANDLE key, int class, void *info, ULONG length))
DEFINE_SYSCALL(NtSetInformationObject, (HANDLE handle, OBJECT_INFORMATION_CLASS info_class, void *ptr, ULONG len))
DEFINE_SYSCALL(NtSetInformationProcess, (HANDLE handle, PROCESSINFOCLASS class, void *info, ULONG size))
DEFINE_SYSCALL(NtSetInformationThread, (HANDLE handle, THREADINFOCLASS class, const void *data, ULONG length))
DEFINE_SYSCALL(NtSetInformationToken, (HANDLE token, TOKEN_INFORMATION_CLASS class, void *info, ULONG length))
DEFINE_SYSCALL(NtSetInformationVirtualMemory, (HANDLE process, VIRTUAL_MEMORY_INFORMATION_CLASS info_class, ULONG_PTR count, PMEMORY_RANGE_ENTRY addresses, PVOID ptr, ULONG size))
DEFINE_SYSCALL(NtSetIntervalProfile, (ULONG interval, KPROFILE_SOURCE source))
DEFINE_SYSCALL(NtSetIoCompletion, (HANDLE handle, ULONG_PTR key, ULONG_PTR value, NTSTATUS status, SIZE_T count))
DEFINE_SYSCALL(NtSetIoCompletionEx, (HANDLE completion_handle, HANDLE completion_reserve_handle, ULONG_PTR key, ULONG_PTR value, NTSTATUS status, SIZE_T count))
DEFINE_SYSCALL(NtSetLdtEntries, (ULONG sel1, LDT_ENTRY entry1, ULONG sel2, LDT_ENTRY entry2))
DEFINE_SYSCALL(NtSetSecurityObject, (HANDLE handle, SECURITY_INFORMATION info, PSECURITY_DESCRIPTOR descr))
DEFINE_SYSCALL(NtSetSystemInformation, (SYSTEM_INFORMATION_CLASS class, void *info, ULONG length))
DEFINE_SYSCALL(NtSetSystemTime, (const LARGE_INTEGER *new, LARGE_INTEGER *old))
DEFINE_SYSCALL(NtSetThreadExecutionState, (EXECUTION_STATE new_state, EXECUTION_STATE *old_state))
DEFINE_SYSCALL(NtSetTimer, (HANDLE handle, const LARGE_INTEGER *when, PTIMER_APC_ROUTINE callback, void *arg, BOOLEAN resume, ULONG period, BOOLEAN *state))
DEFINE_SYSCALL(NtSetTimerResolution, (ULONG res, BOOLEAN set, ULONG *current_res))
DEFINE_SYSCALL(NtSetValueKey, (HANDLE key, const UNICODE_STRING *name, ULONG index, ULONG type, const void *data, ULONG count))
DEFINE_SYSCALL(NtSetVolumeInformationFile, (HANDLE handle, IO_STATUS_BLOCK *io, void *info, ULONG length, FS_INFORMATION_CLASS class))
DEFINE_SYSCALL(NtShutdownSystem, (SHUTDOWN_ACTION action))
DEFINE_SYSCALL(NtSignalAndWaitForSingleObject, (HANDLE signal, HANDLE wait, BOOLEAN alertable, const LARGE_INTEGER *timeout))
DEFINE_SYSCALL(NtSuspendProcess, (HANDLE handle))
DEFINE_SYSCALL(NtSuspendThread, (HANDLE handle, ULONG *count))
DEFINE_SYSCALL(NtSystemDebugControl, (SYSDBG_COMMAND command, void *in_buff, ULONG in_len, void *out_buff, ULONG out_len, ULONG *retlen))
DEFINE_SYSCALL(NtTerminateJobObject, (HANDLE handle, NTSTATUS status))
DEFINE_WRAPPED_SYSCALL(NtTerminateProcess, (HANDLE handle, LONG exit_code))
DEFINE_WRAPPED_SYSCALL(NtTerminateThread, (HANDLE handle, LONG exit_code))
DEFINE_SYSCALL(NtTestAlert, (void))
DEFINE_SYSCALL(NtTraceControl, (ULONG code, void *inbuf, ULONG inbuf_len, void *outbuf, ULONG outbuf_len, ULONG *size))
DEFINE_SYSCALL(NtTraceEvent, (HANDLE handle, ULONG flags, ULONG size, void *data))
DEFINE_SYSCALL(NtUnloadDriver, (const UNICODE_STRING *name))
DEFINE_SYSCALL(NtUnloadKey, (OBJECT_ATTRIBUTES *attr))
DEFINE_SYSCALL(NtUnlockFile, (HANDLE handle, IO_STATUS_BLOCK *io_status, LARGE_INTEGER *offset, LARGE_INTEGER *count, ULONG *key))
DEFINE_SYSCALL(NtUnlockVirtualMemory, (HANDLE process, PVOID *addr, SIZE_T *size, ULONG unknown))
DEFINE_WRAPPED_SYSCALL(NtUnmapViewOfSection, (HANDLE process, PVOID addr))
DEFINE_WRAPPED_SYSCALL(NtUnmapViewOfSectionEx, (HANDLE process, PVOID addr, ULONG flags))
DEFINE_SYSCALL(NtWaitForAlertByThreadId, (const void *address, const LARGE_INTEGER *timeout))
DEFINE_SYSCALL(NtWaitForDebugEvent, (HANDLE handle, BOOLEAN alertable, LARGE_INTEGER *timeout, DBGUI_WAIT_STATE_CHANGE *state))
DEFINE_SYSCALL(NtWaitForKeyedEvent, (HANDLE handle, const void *key, BOOLEAN alertable, const LARGE_INTEGER *timeout))
DEFINE_SYSCALL(NtWaitForMultipleObjects, (DWORD count, const HANDLE *handles, WAIT_TYPE type, BOOLEAN alertable, const LARGE_INTEGER *timeout))
DEFINE_SYSCALL(NtWaitForMultipleObjects32, (ULONG count, LONG *handles, WAIT_TYPE type, BOOLEAN alertable, const LARGE_INTEGER *timeout))
DEFINE_SYSCALL(NtWaitForSingleObject, (HANDLE handle, BOOLEAN alertable, const LARGE_INTEGER *timeout))
DEFINE_SYSCALL(NtWorkerFactoryWorkerReady, (HANDLE handle))
DEFINE_SYSCALL(NtWriteFile, (HANDLE handle, HANDLE event, PIO_APC_ROUTINE apc, void *apc_user, IO_STATUS_BLOCK *io, const void *buffer, ULONG length, LARGE_INTEGER *offset, ULONG *key))
DEFINE_SYSCALL(NtWriteFileGather, (HANDLE file, HANDLE event, PIO_APC_ROUTINE apc, void *apc_user, IO_STATUS_BLOCK *io, FILE_SEGMENT_ELEMENT *segments, ULONG length, LARGE_INTEGER *offset, ULONG *key))
DEFINE_SYSCALL(NtWriteRequestData, (HANDLE handle, LPC_MESSAGE *request, ULONG id, void *buffer, ULONG len, ULONG *retlen))
DEFINE_SYSCALL(NtWriteVirtualMemory, (HANDLE process, void *addr, const void *buffer, SIZE_T size, SIZE_T *bytes_written))
DEFINE_SYSCALL(NtYieldExecution, (void))

NTSTATUS SYSCALL_API NtAllocateVirtualMemory( HANDLE process, PVOID *ret, ULONG_PTR zero_bits,
                                              SIZE_T *size_ptr, ULONG type, ULONG protect )
{
    BOOL is_current = RtlIsCurrentProcess( process );
    NTSTATUS status;

    if (!enter_syscall_callback())
        return syscall_NtAllocateVirtualMemory( process, ret, zero_bits, size_ptr, type, protect );

    if (!*ret && (type & MEM_COMMIT)) type |= MEM_RESERVE;

    if (!is_current) send_cross_process_notification( process, CrossProcessPreVirtualAlloc,
                                                      *ret, *size_ptr, 3, type, protect, 0 );
    else if (pNotifyMemoryAlloc) pNotifyMemoryAlloc( *ret, *size_ptr, type, protect, FALSE, 0 );

    status = syscall_NtAllocateVirtualMemory( process, ret, zero_bits, size_ptr, type, protect );

    if (!is_current) send_cross_process_notification( process, CrossProcessPostVirtualAlloc,
                                                      *ret, *size_ptr, 3, type, protect, status );
    else if (pNotifyMemoryAlloc) pNotifyMemoryAlloc( *ret, *size_ptr, type, protect, TRUE, status );

    leave_syscall_callback();
    return status;
}

NTSTATUS SYSCALL_API NtAllocateVirtualMemoryEx( HANDLE process, PVOID *ret, SIZE_T *size_ptr, ULONG type,
                                                ULONG protect, MEM_EXTENDED_PARAMETER *parameters, ULONG count )
{
    BOOL is_current = RtlIsCurrentProcess( process );
    NTSTATUS status;

    if (!enter_syscall_callback())
        return syscall_NtAllocateVirtualMemoryEx( process, ret, size_ptr, type, protect, parameters, count );

    if (!*ret && (type & MEM_COMMIT)) type |= MEM_RESERVE;

    if (!is_current) send_cross_process_notification( process, CrossProcessPreVirtualAlloc,
                                                      *ret, *size_ptr, 3, type, protect, 0 );
    else if (pNotifyMemoryAlloc) pNotifyMemoryAlloc( *ret, *size_ptr, type, protect, FALSE, 0 );

    status = syscall_NtAllocateVirtualMemoryEx( process, ret, size_ptr, type, protect, parameters, count );

    if (!is_current) send_cross_process_notification( process, CrossProcessPostVirtualAlloc,
                                                      *ret, *size_ptr, 3, type, protect, status );
    else if (pNotifyMemoryAlloc) pNotifyMemoryAlloc( *ret, *size_ptr, type, protect, TRUE, status );

    leave_syscall_callback();
    return status;
}

NTSTATUS SYSCALL_API NtContinue( CONTEXT *context, BOOLEAN alertable )
{
    ARM64_NT_CONTEXT arm_ctx;

    context_x64_to_arm( &arm_ctx, (ARM64EC_NT_CONTEXT *)context );
    return syscall_NtContinue( &arm_ctx, alertable );
}

NTSTATUS SYSCALL_API NtContinueEx( CONTEXT *context, KCONTINUE_ARGUMENT *args )
{
    ARM64_NT_CONTEXT arm_ctx;

    context_x64_to_arm( &arm_ctx, (ARM64EC_NT_CONTEXT *)context );
    return syscall_NtContinueEx( &arm_ctx, args );
}

NTSTATUS SYSCALL_API NtFlushInstructionCache( HANDLE process, const void *addr, SIZE_T size )
{
    NTSTATUS status = syscall_NtFlushInstructionCache( process, addr, size );

    if (!status && enter_syscall_callback())
    {
        if (!RtlIsCurrentProcess( process ))
            send_cross_process_notification( process, CrossProcessFlushCache, addr, size, 0 );
        else if (pBTCpu64FlushInstructionCache)
            pBTCpu64FlushInstructionCache( addr, size );
        leave_syscall_callback();
    }
    return status;
}

NTSTATUS SYSCALL_API NtFreeVirtualMemory( HANDLE process, PVOID *addr_ptr, SIZE_T *size_ptr, ULONG type )
{
    BOOL is_current = RtlIsCurrentProcess( process );
    NTSTATUS status;

    if (!enter_syscall_callback())
        return syscall_NtFreeVirtualMemory( process, addr_ptr, size_ptr, type );

    if (!is_current) send_cross_process_notification( process, CrossProcessPreVirtualFree,
                                                      *addr_ptr, *size_ptr, 2, type, 0 );
    else if (pNotifyMemoryFree) pNotifyMemoryFree( *addr_ptr, *size_ptr, type, FALSE, 0 );

    status = syscall_NtFreeVirtualMemory( process, addr_ptr, size_ptr, type );

    if (!is_current) send_cross_process_notification( process, CrossProcessPostVirtualFree,
                                                      *addr_ptr, *size_ptr, 2, type, status );
    else if (pNotifyMemoryFree) pNotifyMemoryFree( *addr_ptr, *size_ptr, type, TRUE, status );

    leave_syscall_callback();
    return status;
}

NTSTATUS SYSCALL_API NtGetContextThread( HANDLE handle, CONTEXT *context )
{
    ARM64_NT_CONTEXT arm_ctx = { .ContextFlags = ctx_flags_x64_to_arm( context->ContextFlags ) };
    NTSTATUS status = syscall_NtGetContextThread( handle, &arm_ctx );

    if (!status) context_arm_to_x64( (ARM64EC_NT_CONTEXT *)context, &arm_ctx );
    return status;
}

static void notify_map_view_of_section( HANDLE handle, void *addr, SIZE_T size, ULONG alloc,
                                        ULONG protect, NTSTATUS *ret_status )
{
    SECTION_IMAGE_INFORMATION info;
    NTSTATUS status;

    if (!pNotifyMapViewOfSection) return;
    if (!NtCurrentTeb()->Tib.ArbitraryUserPointer) return;
    if (NtQuerySection( handle, SectionImageInformation, &info, sizeof(info), NULL )) return;
    status = pNotifyMapViewOfSection( NULL, addr, NULL, size, alloc, protect );
    if (NT_SUCCESS(status)) return;
    NtUnmapViewOfSection( GetCurrentProcess(), addr );
    *ret_status = status;
}

NTSTATUS SYSCALL_API NtMapViewOfSection( HANDLE handle, HANDLE process, PVOID *addr_ptr,
                                         ULONG_PTR zero_bits, SIZE_T commit_size,
                                         const LARGE_INTEGER *offset, SIZE_T *size_ptr,
                                         SECTION_INHERIT inherit, ULONG alloc_type, ULONG protect )
{
    NTSTATUS status = syscall_NtMapViewOfSection( handle, process, addr_ptr, zero_bits, commit_size,
                                                  offset, size_ptr, inherit, alloc_type, protect );

    if (NT_SUCCESS(status) && RtlIsCurrentProcess( process ) && enter_syscall_callback())
    {
        notify_map_view_of_section( handle, *addr_ptr, *size_ptr, alloc_type, protect, &status );
        leave_syscall_callback();
    }
    return status;
}

NTSTATUS SYSCALL_API NtMapViewOfSectionEx( HANDLE handle, HANDLE process, PVOID *addr_ptr,
                                           const LARGE_INTEGER *offset, SIZE_T *size_ptr, ULONG alloc_type,
                                           ULONG protect, MEM_EXTENDED_PARAMETER *parameters, ULONG count )
{
    NTSTATUS status = syscall_NtMapViewOfSectionEx( handle, process, addr_ptr, offset, size_ptr,
                                                    alloc_type, protect, parameters, count );

    if (NT_SUCCESS(status) && RtlIsCurrentProcess( process ) && enter_syscall_callback())
    {
        notify_map_view_of_section( handle, *addr_ptr, *size_ptr, alloc_type, protect, &status );
        leave_syscall_callback();
    }
    return status;
}

NTSTATUS SYSCALL_API NtProtectVirtualMemory( HANDLE process, PVOID *addr_ptr, SIZE_T *size_ptr,
                                             ULONG new_prot, ULONG *old_prot )
{
    BOOL is_current = RtlIsCurrentProcess( process );
    NTSTATUS status;

    if (!enter_syscall_callback())
        return syscall_NtProtectVirtualMemory( process, addr_ptr, size_ptr, new_prot, old_prot );

    if (!is_current) send_cross_process_notification( process, CrossProcessPreVirtualProtect,
                                                      *addr_ptr, *size_ptr, 2, new_prot, 0 );
    else if (pNotifyMemoryProtect) pNotifyMemoryProtect( *addr_ptr, *size_ptr, new_prot, FALSE, 0 );

    status = syscall_NtProtectVirtualMemory( process, addr_ptr, size_ptr, new_prot, old_prot );

    if (!is_current) send_cross_process_notification( process, CrossProcessPostVirtualProtect,
                                                      *addr_ptr, *size_ptr, 2, new_prot, status );
    else if (pNotifyMemoryProtect) pNotifyMemoryProtect( *addr_ptr, *size_ptr, new_prot, TRUE, status );

    leave_syscall_callback();
    return status;
}

NTSTATUS SYSCALL_API NtQuerySystemInformation( SYSTEM_INFORMATION_CLASS class, void *info, ULONG size, ULONG *ret_size )
{
    NTSTATUS status = syscall_NtQuerySystemInformation( class, info, size, ret_size );

    if (!status && class == SystemCpuInformation && pUpdateProcessorInformation) pUpdateProcessorInformation( info );
    return status;
}

NTSTATUS SYSCALL_API NtRaiseException( EXCEPTION_RECORD *rec, CONTEXT *context, BOOL first_chance )
{
    ARM64_NT_CONTEXT arm_ctx;

    context_x64_to_arm( &arm_ctx, (ARM64EC_NT_CONTEXT *)context );
    return syscall_NtRaiseException( rec, &arm_ctx, first_chance );
}

NTSTATUS SYSCALL_API NtReadFile( HANDLE handle, HANDLE event, PIO_APC_ROUTINE apc, void *apc_user,
                                 IO_STATUS_BLOCK *io, void *buffer, ULONG length,
                                 LARGE_INTEGER *offset, ULONG *key )
{
    NTSTATUS status;

    if (pBTCpu64NotifyReadFile && enter_syscall_callback())
    {
        pBTCpu64NotifyReadFile( handle, buffer, length, FALSE, 0 );
        status = syscall_NtReadFile( handle, event, apc, apc_user, io, buffer, length, offset, key );
        if (pBTCpu64NotifyReadFile) pBTCpu64NotifyReadFile( handle, buffer, length, TRUE, status );
        leave_syscall_callback();
        return status;
    }
    return syscall_NtReadFile( handle, event, apc, apc_user, io, buffer, length, offset, key );
}

NTSTATUS SYSCALL_API NtSetContextThread( HANDLE handle, const CONTEXT *context )
{
    ARM64_NT_CONTEXT arm_ctx;

    context_x64_to_arm( &arm_ctx, (ARM64EC_NT_CONTEXT *)context );
    return syscall_NtSetContextThread( handle, &arm_ctx );
}

NTSTATUS SYSCALL_API NtTerminateProcess( HANDLE handle, LONG exit_code )
{
    NTSTATUS status;

    if (!handle && pProcessTerm && enter_syscall_callback())
    {
        pProcessTerm( handle, FALSE, 0 );
        status = syscall_NtTerminateProcess( handle, exit_code );
        pProcessTerm( handle, TRUE, status );
        leave_syscall_callback();
        return status;
    }
    return syscall_NtTerminateProcess( handle, exit_code );
}

NTSTATUS SYSCALL_API NtTerminateThread( HANDLE handle, LONG exit_code )
{
    NTSTATUS status;

    if (pThreadTerm && enter_syscall_callback())
    {
        pThreadTerm( handle, exit_code );
        status = syscall_NtTerminateThread( handle, exit_code );
        leave_syscall_callback();
        return status;
    }
    return syscall_NtTerminateThread( handle, exit_code );
}

NTSTATUS SYSCALL_API NtUnmapViewOfSection( HANDLE process, void *addr )
{
    BOOL is_current = RtlIsCurrentProcess( process );
    NTSTATUS status;

    if (is_current && pNotifyUnmapViewOfSection && enter_syscall_callback())
    {
        pNotifyUnmapViewOfSection( addr, FALSE, 0 );
        status = syscall_NtUnmapViewOfSection( process, addr );
        pNotifyUnmapViewOfSection( addr, TRUE, status );
        leave_syscall_callback();
        return status;
    }
    return syscall_NtUnmapViewOfSection( process, addr );
}

NTSTATUS SYSCALL_API NtUnmapViewOfSectionEx( HANDLE process, void *addr, ULONG flags )
{
    BOOL is_current = RtlIsCurrentProcess( process );
    NTSTATUS status;

    if (is_current && pNotifyUnmapViewOfSection && enter_syscall_callback())
    {
        pNotifyUnmapViewOfSection( addr, FALSE, 0 );
        status = syscall_NtUnmapViewOfSectionEx( process, addr, flags );
        pNotifyUnmapViewOfSection( addr, TRUE, status );
        leave_syscall_callback();
        return status;
    }
    return syscall_NtUnmapViewOfSectionEx( process, addr, flags );
}


asm( ".section .rdata, \"dr\"\n\t"
     ".balign 8\n\t"
     ".globl arm64ec_syscalls\n"
     "arm64ec_syscalls:\n\t"
#define SYSCALL_ENTRY(id,name,args) ".quad \"#" #name "$hp_target\"\n\t"
     ALL_SYSCALLS
#undef SYSCALL_ENTRY
     ".text" );


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
    case 2:
        *(UINT *)output = 0x27f;  /* hard-coded x87 control word */
        return STATUS_SUCCESS;
    default:
        FIXME( "not implemented type %lu\n", type );
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
        FIXME( "not implemented type %lu\n", type );
        return STATUS_INVALID_PARAMETER;
    }
}


/**********************************************************************
 *           ProcessPendingCrossProcessEmulatorWork  (ntdll.@)
 */
void WINAPI ProcessPendingCrossProcessEmulatorWork(void)
{
    CHPEV2_PROCESS_INFO *info = RtlGetCurrentPeb()->ChpeV2ProcessInfo;
    CROSS_PROCESS_WORK_LIST *list = (void *)info->CrossProcessWorkList;
    CROSS_PROCESS_WORK_ENTRY *entry;
    BOOLEAN flush = FALSE;
    UINT next;

    if (!list) return;
    entry = RtlWow64PopAllCrossProcessWorkFromWorkList( &list->work_list, &flush );

    if (flush)
    {
        if (pFlushInstructionCacheHeavy) pFlushInstructionCacheHeavy( NULL, 0 );
        while (entry)
        {
            next = entry->next;
            RtlWow64PushCrossProcessWorkOntoFreeList( &list->free_list, entry );
            entry = next ? CROSS_PROCESS_LIST_ENTRY( &list->work_list, next ) : NULL;
        }
        return;
    }

    while (entry)
    {
        switch (entry->id)
        {
        case CrossProcessPreVirtualAlloc:
        case CrossProcessPostVirtualAlloc:
            if (!pNotifyMemoryAlloc) break;
            pNotifyMemoryAlloc( (void *)entry->addr, entry->size, entry->args[0], entry->args[1],
                                entry->id == CrossProcessPostVirtualAlloc, entry->args[2] );
            break;
        case CrossProcessPreVirtualFree:
        case CrossProcessPostVirtualFree:
            if (!pNotifyMemoryFree) break;
            pNotifyMemoryFree( (void *)entry->addr, entry->size, entry->args[0],
                               entry->id == CrossProcessPostVirtualFree, entry->args[1] );
            break;
        case CrossProcessPreVirtualProtect:
        case CrossProcessPostVirtualProtect:
            if (!pNotifyMemoryProtect) break;
            pNotifyMemoryProtect( (void *)entry->addr, entry->size, entry->args[0],
                                  entry->id == CrossProcessPostVirtualProtect, entry->args[1] );
            break;
        case CrossProcessFlushCache:
            if (!pBTCpu64FlushInstructionCache) break;
            pBTCpu64FlushInstructionCache( (void *)entry->addr, entry->size );
            break;
        case CrossProcessFlushCacheHeavy:
            if (!pFlushInstructionCacheHeavy) break;
            pFlushInstructionCacheHeavy( (void *)entry->addr, entry->size );
            break;
        case CrossProcessMemoryWrite:
            if (!pBTCpu64NotifyMemoryDirty) break;
            pBTCpu64NotifyMemoryDirty( (void *)entry->addr, entry->size );
            break;
        }
        next = entry->next;
        RtlWow64PushCrossProcessWorkOntoFreeList( &list->free_list, entry );
        entry = next ? CROSS_PROCESS_LIST_ENTRY( &list->work_list, next ) : NULL;
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


/*******************************************************************
 *         nested_exception_handler
 */
EXCEPTION_DISPOSITION WINAPI nested_exception_handler( EXCEPTION_RECORD *rec, void *frame,
                                                       CONTEXT *context, void *dispatch )
{
    if (rec->ExceptionFlags & (EXCEPTION_UNWINDING | EXCEPTION_EXIT_UNWIND)) return ExceptionContinueSearch;
    return ExceptionNestedException;
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
 *		KiUserEmulationDispatcher (NTDLL.@)
 */
void dispatch_emulation( ARM64_NT_CONTEXT *arm_ctx )
{
    context_arm_to_x64( get_arm64ec_cpu_area()->ContextAmd64, arm_ctx );
    get_arm64ec_cpu_area()->InSimulation = 1;
    pBeginSimulation();
}
__ASM_GLOBAL_FUNC( "#KiUserEmulationDispatcher",
                   ".seh_context\n\t"
                   ".seh_endprologue\n\t"
                   "mov x0, sp\n\t"   /* context */
                   "bl dispatch_emulation\n\t"
                   "brk #1" )


/*******************************************************************
 *		dispatch_syscall
 */
static void dispatch_syscall( ARM64_NT_CONTEXT *context )
{
    if (context->X8 < __nb_syscalls)  /* syscall number in rax */
    {
        context->X0 = context->X4;  /* get first param from r10 */
        context->X4 = context->Pc;  /* and save return address to syscall thunk */
        context->Pc = (ULONG_PTR)invoke_arm64ec_syscall;
    }
    else context->X8 = STATUS_INVALID_SYSTEM_SERVICE;  /* set return value in rax */

    /* return to x64 code so that the syscall entry thunk is invoked properly */
    dispatch_emulation( context );
}


static void * __attribute__((used)) prepare_exception_arm64ec( EXCEPTION_RECORD *rec, ARM64EC_NT_CONTEXT *context, ARM64_NT_CONTEXT *arm_ctx )
{
    if (rec->ExceptionCode == STATUS_EMULATION_SYSCALL) dispatch_syscall( arm_ctx );
    context_arm_to_x64( context, arm_ctx );
    if (pResetToConsistentState) pResetToConsistentState( rec, &context->AMD64_Context, arm_ctx );
    /* call x64 dispatcher if the thunk or the function pointer was modified */
    if (pWow64PrepareForException || memcmp( KiUserExceptionDispatcher_thunk, KiUserExceptionDispatcher_orig,
                                             sizeof(KiUserExceptionDispatcher_orig) ))
        return KiUserExceptionDispatcher_thunk;
    return NULL;
}

/*******************************************************************
 *		KiUserExceptionDispatcher (NTDLL.@)
 */
void __attribute__((naked)) KiUserExceptionDispatcher( EXCEPTION_RECORD *rec, CONTEXT *context )
{
    asm( ".seh_proc \"#KiUserExceptionDispatcher\"\n\t"
         ".seh_context\n\t"
         "sub sp, sp, #0x4d0\n\t"       /* sizeof(ARM64EC_NT_CONTEXT) */
         ".seh_stackalloc 0x4d0\n\t"
         ".seh_endprologue\n\t"
         "add x0, sp, #0x3b0+0x4d0\n\t" /* rec */
         "mov x1, sp\n\t"               /* context */
         "add x2, sp, #0x4d0\n\t"       /* arm_ctx (context + 1) */
         "bl \"#prepare_exception_arm64ec\"\n\t"
         "cbz x0, 1f\n\t"
         /* bypass exit thunk to avoid messing up the stack */
         "adrp x16, __os_arm64x_dispatch_call_no_redirect\n\t"
         "ldr x16, [x16, #:lo12:__os_arm64x_dispatch_call_no_redirect]\n\t"
         "mov x9, x0\n\t"
         "blr x16\n"
         "1:\tadd x0, sp, #0x3b0+0x4d0\n\t" /* rec */
         "mov x1, sp\n\t"                   /* context */
         "bl #dispatch_exception\n\t"
         "brk #1\n\t"
         ".seh_endproc" );
}


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
                   "bl \"#dispatch_apc\"\n\t"
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
                   ".seh_handler user_callback_handler, @except\n\t"
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
                   "bl \"#NtCallbackReturn\"\n\t"
                   "bl \"#RtlRaiseStatus\"\n\t"
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
    asm( ".seh_proc \"#RtlCaptureContext\"\n\t"
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
         "b \"#capture_context\"\n\t"
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
    asm( ".seh_proc \"#NTDLL__setjmpex\"\n\t"
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
         "b \"#do_setjmpex\"\n\t"
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
    while (is_valid_frame( (ULONG_PTR)teb_frame ) && (ULONG64)teb_frame < context->Rsp)
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
 *		RtlGetNativeSystemInformation (NTDLL.@)
 */
NTSTATUS WINAPI RtlGetNativeSystemInformation( SYSTEM_INFORMATION_CLASS class,
                                               void *info, ULONG size, ULONG *ret_size )
{
    return syscall_NtQuerySystemInformation( class, info, size, ret_size );
}


/***********************************************************************
 *           RtlIsProcessorFeaturePresent [NTDLL.@]
 */
BOOLEAN WINAPI RtlIsProcessorFeaturePresent( UINT feature )
{
    static const ULONGLONG arm64_features =
        (1ull << PF_COMPARE_EXCHANGE_DOUBLE) |
        (1ull << PF_NX_ENABLED) |
        (1ull << PF_ARM_VFP_32_REGISTERS_AVAILABLE) |
        (1ull << PF_ARM_NEON_INSTRUCTIONS_AVAILABLE) |
        (1ull << PF_SECOND_LEVEL_ADDRESS_TRANSLATION) |
        (1ull << PF_FASTFAIL_AVAILABLE) |
        (1ull << PF_ARM_DIVIDE_INSTRUCTION_AVAILABLE) |
        (1ull << PF_ARM_64BIT_LOADSTORE_ATOMIC) |
        (1ull << PF_ARM_EXTERNAL_CACHE_AVAILABLE) |
        (1ull << PF_ARM_FMAC_INSTRUCTIONS_AVAILABLE) |
        (1ull << PF_ARM_V8_INSTRUCTIONS_AVAILABLE) |
        (1ull << PF_ARM_V8_CRYPTO_INSTRUCTIONS_AVAILABLE) |
        (1ull << PF_ARM_V8_CRC32_INSTRUCTIONS_AVAILABLE) |
        (1ull << PF_ARM_V81_ATOMIC_INSTRUCTIONS_AVAILABLE) |
        (1ull << PF_ARM_V82_DP_INSTRUCTIONS_AVAILABLE) |
        (1ull << PF_ARM_V83_JSCVT_INSTRUCTIONS_AVAILABLE) |
        (1ull << PF_ARM_V83_LRCPC_INSTRUCTIONS_AVAILABLE);

    if (feature >= PROCESSOR_FEATURE_MAX) return FALSE;
    if (arm64_features & (1ull << feature)) return user_shared_data->ProcessorFeatures[feature];
    return emulated_processor_features[feature];
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


/*************************************************************************
 *		__wine_unix_call_arm64ec
 */
NTSTATUS __attribute__((naked)) __wine_unix_call_arm64ec( unixlib_handle_t handle, unsigned int code, void *args )
{
    asm( ".seh_proc \"#__wine_unix_call_arm64ec\"\n\t"
         ".seh_endprologue\n\t"
         "adrp x16, __wine_unix_call_dispatcher_arm64ec\n\t"
         "ldr x16, [x16, #:lo12:__wine_unix_call_dispatcher_arm64ec]\n\t"
         "br x16\n\t"
         ".seh_endproc" );
}

NTSTATUS (WINAPI *__wine_unix_call_dispatcher_arm64ec)( unixlib_handle_t, unsigned int, void * ) = __wine_unix_call_arm64ec;

static void __attribute__((naked)) arm64x_check_call_early(void)
{
    asm( "mov x11, x9\n\t"
         "ret" );
}

static void __attribute__((naked)) arm64x_check_icall_early(void)
{
    asm( "ret" );
}

/**************************************************************************
 *		arm64x_check_call
 *
 * Implementation of __os_arm64x_check_call.
 */
static void __attribute__((naked)) arm64x_check_call(void)
{
    asm( ".seh_proc \"#arm64x_check_call\"\n\t"
         ".seh_endprologue\n\t"
         /* check for EC code */
         "ldr x16, [x18, #0x60]\n\t"        /* peb */
         "lsr x17, x11, #18\n\t"            /* dest / page_size / 64 */
         "ldr x16, [x16, #0x368]\n\t"       /* peb->EcCodeBitMap */
         "lsr x9, x11, #12\n\t"             /* dest / page_size */
         "ldr x16, [x16, x17, lsl #3]\n\t"
         "lsr x16, x16, x9\n\t"
         "tbnz x16, #0, .Ldone\n\t"
         /* check if dest is aligned */
         "tst x11, #15\n\t"                 /* dest & 15 */
         "b.ne .Ljmp\n\t"
         "ldr x16, [x11]\n\t"               /* dest code */
         /* check for fast-forward sequence */
         "ldr x17, .Lffwd_seq\n\t"
         "cmp x16, x17\n\t"
         "b.ne .Lsyscall\n\t"
         "ldr x16, [x11, #8]\n\t"
         "ldr x17, .Lffwd_seq + 8\n\t"
         "eor x17, x16, x17\n\t"            /* compare only first two bytes */
         "tst x17, #0xffff\n\t"
         "b.ne .Lexit\n\t"
         "add x11, x11, #14\n\t"            /* address after jump */
         "lsr x9, x16, #16\n\t"
         "add x11, x11, w9, sxtw\n\t"       /* add offset */
         "ret\n"
         /* check for syscall sequence */
         ".Lsyscall:\n\t"
         "ldr w17, .Lsyscall_seq\n\t"
         "cmp w16, w17\n\t"
         "b.ne .Ljmp\n\t"
         "ubfx x9, x16, #32, #12\n\t"       /* syscall number */
         "ldr x16, [x11, #8]\n\t"
         "ldr x17, .Lsyscall_seq + 8\n\t"
         "cmp x16, x17\n\t"
         "b.ne .Lexit\n\t"
         "ldr x16, [x11, #16]\n\t"
         "ldr x17, .Lsyscall_seq + 16\n\t"
         "cmp x16, x17\n\t"
         "b.ne .Lexit\n\t"
         "cmp x9, #%0\n\t"
         "b.hs .Lexit\n\t"
         "adr x11, arm64ec_syscalls\n\t"
         "ldr x11, [x11, x9, lsl #3]\n\t"
         "ret\n"
         /* check for jmp sequence */
         ".Ljmp:\n\t"
         "ldrb w16, [x11]\n\t"
         "cmp w16, #0xff\n\t"
         "b.ne .Lexit\n\t"
         "ldrb w16, [x11, #1]\n\t"
         "cmp w16, #0x25\n\t"               /* ff 25 jmp *xxx(%rip) */
         "b.ne .Lexit\n\t"
         "ldr w9, [x11, #2]\n\t"
         "add x16, x11, #6\n\t"             /* address after jump */
         "ldr x11, [x16, w9, sxtw]\n\t"
         "b \"#arm64x_check_call\"\n"       /* restart checks with jump destination */
         /* not a special sequence, call the exit thunk */
         ".Lexit:\n\t"
         "mov x9, x11\n\t"
         "mov x11, x10\n\t"
         ".Ldone:\n\t"
         "ret\n"
         ".seh_endproc\n\t"

         ".Lffwd_seq:\n\t"
         ".byte 0x48, 0x8b, 0xc4\n\t"       /* mov  %rsp,%rax */
         ".byte 0x48, 0x89, 0x58, 0x20\n\t" /* mov  %rbx,0x20(%rax) */
         ".byte 0x55\n"                     /* push %rbp */
         ".byte 0x5d\n\t"                   /* pop  %rbp */
         ".byte 0xe9\n\t"                   /* jmp  arm_code */
         ".byte 0, 0, 0, 0, 0, 0\n"

         ".Lsyscall_seq:\n\t"
         ".byte 0x4c, 0x8b, 0xd1\n\t"       /* mov  %rcx,%r10 */
         ".byte 0xb8, 0, 0, 0, 0\n\t"       /* mov  $xxx,%eax */
         ".byte 0xf6, 0x04, 0x25, 0x08\n\t" /* testb $0x1,0x7ffe0308 */
         ".byte 0x03, 0xfe, 0x7f, 0x01\n\t"
         ".byte 0x75, 0x03\n\t"             /* jne 1f */
         ".byte 0x0f, 0x05\n\t"             /* syscall */
         ".byte 0xc3\n\t"                   /* ret */
         ".byte 0xcd, 0x2e\n\t"             /* 1: int $0x2e */
         ".byte 0xc3"                       /* ret */
         :: "i" (__nb_syscalls) );
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
    asm( ".seh_proc \"#RtlRaiseException\"\n\t"
         "sub sp, sp, #0x4d0\n\t"     /* sizeof(context) */
         ".seh_stackalloc 0x4d0\n\t"
         "stp x29, x30, [sp, #-0x20]!\n\t"
         ".seh_save_fplr_x 0x20\n\t"
         "str x0, [sp, #0x10]\n\t"
         ".seh_save_any_reg x0, 0x10\n\t"
         ".seh_endprologue\n\t"
         "add x0, sp, #0x20\n\t"
         "bl \"#RtlCaptureContext\"\n\t"
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
         "bl \"#dispatch_exception\"\n"
         "1:\tmov w2, #1\n\t"
         "bl \"#NtRaiseException\"\n\t"
         "b \"#RtlRaiseStatus\"\n\t" /* does not return */
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
    asm( ".seh_proc \"#RtlUserThreadStart\"\n\t"
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
        __os_arm64x_check_call = arm64x_check_call_early;
        __os_arm64x_check_icall = arm64x_check_icall_early;
        __os_arm64x_check_icall_cfg = arm64x_check_icall_early;
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
    asm( ".seh_proc \"#DbgUiRemoteBreakin\"\n\t"
         "stp x29, x30, [sp, #-16]!\n\t"
         ".seh_save_fplr_x 16\n\t"
         ".seh_endprologue\n\t"
         ".seh_handler DbgUiRemoteBreakin_handler, @except\n\t"
         "ldr x0, [x18, #0x60]\n\t"  /* NtCurrentTeb()->Peb */
         "ldrb w0, [x0, 0x02]\n\t"   /* peb->BeingDebugged */
         "cbz w0, 1f\n\t"
         "bl \"#DbgBreakPoint\"\n"
         "1:\tmov w0, #0\n\t"
         "bl \"#RtlExitUserThread\"\n"
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
    asm( ".seh_proc \"#DbgBreakPoint\"\n\t"
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
    asm( ".seh_proc \"#DbgUserBreakPoint\"\n\t"
         ".seh_endprologue\n\t"
         "brk #0xf000\n\t"
         "ret\n\t"
         ".seh_endproc" );
}

#endif  /* __arm64ec__ */
